const gulp = require("gulp");
const uglify = require("gulp-uglify");
const cssnano = require("gulp-cssnano");
const through = require("through2");
const path = require("path");
const fs = require("fs");
const readline = require("readline");

const destination = "new_http.c";

function dumpFileSize() {
  return through.obj(function (file, enc, cb) {
    console.log(
      `Processing ${file.basename}, original length ${file.stat.size}`
    );
    cb(null, file);
  });
}

/** This function injects C for a const field in new_http.c */
function generateCode(field_name, is_script) {
  return through.obj(function (file, enc, cb) {
    if (file.isBuffer()) {
      const contents = file.contents;

      let output = String(contents);
      output = output.replace(/\"/g, '\\"');
      console.log(
        `Processing ${file.basename}, reduced length ${contents.length}`
      );

      const prefix = is_script ? "<script type='text/javascript'>" : "<style>";
      const suffix = is_script ? "</script>" : "</style>";
      output = `const char ${field_name}[] = "${prefix}${output}${suffix}";`;

      const target_path = path.join(path.dirname(file.path), destination);
      //console.log(`Updated ${target_path}`);

      const rl = readline.createInterface({
        input: fs.createReadStream(target_path),
        crlfDelay: Infinity,
      });

      const merged_contents = [];
      const marker_start = `//region_start ${field_name}`;
      const marker_end = `//region_end ${field_name}`;
      let region_state = 0;

      rl.on("line", (line) => {
        if (line.trim() === marker_start) {
          region_state = 1;
          merged_contents.push(marker_start);
          merged_contents.push(output);
          merged_contents.push(marker_end);
        } else {
          //Skip all existing content lines till region ends
          if (region_state === 1) {
            if (line.trim() === marker_end) {
              region_state = 2;
            }
          } else {
            merged_contents.push(line);
          }
        }
      });

      rl.on("close", () => {
        if (region_state === 0) {
          //Starting marker was not found, append

          merged_contents.push("");
          merged_contents.push(marker_start);
          merged_contents.push(output);
          merged_contents.push(marker_end);
        }

        if (region_state === 1) {
          cb(`Ending marker "${marker_end}" was not found.`, file);
        } else {
          fs.writeFile(
            target_path,
            merged_contents.join("\r\n"),
            "utf8",
            (err) => {
              cb(err, file);
            }
          );
        }
      });

      return;
    }

    cb(null, file);
  });
}

function minifyJs() {
  return gulp
    .src("./src/httpserver/script.js")
    .pipe(dumpFileSize())
    .pipe(uglify())
    .pipe(generateCode("pageScript", true));
}

function minifyHassDiscoveryJs() {
  return gulp
    .src("./src/httpserver/script_ha_discovery.js")
    .pipe(dumpFileSize())
    .pipe(uglify())
    .pipe(generateCode("ha_discovery_script", true));
}

function minifyCss() {
  return gulp
    .src("./src/httpserver/style.css")
    .pipe(dumpFileSize())
    .pipe(cssnano())
    .pipe(generateCode("htmlHeadStyle", false));
}

exports.default = gulp.series(minifyJs, minifyHassDiscoveryJs, minifyCss);
