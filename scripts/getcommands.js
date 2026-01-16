let fs = require('fs');

let commands = [];
let cmdindex = {};
let channels = [];
let chanindex = {};
let ios = [];
let ioindex = {};
let drvs = [];
let drvindex = {};
let drvdefines = {};	// try to save the "#if ENABLE_DRIVER_XY"
let flags = [];
let flagindex = {};
let cnsts = [];
let cnstindex = {};
let evnts = [];

let inFile = 0;

let oldStyleFlagTitles = [];


function mytrim(text) {
	text = text.trim();
	if (text.startsWith('"')) text = text.slice(1, -1);
	return text;
}

function writeIfChanged(fname, data) {
	if (fs.existsSync(fname)) {
		const existing = fs.readFileSync(fname, "utf8");
		if (existing.replace(/\r/g, "") === data.replace(/\r/g, "")) {
			// console.log("Skipping " + fname + " - no changes");
			return;
		}
	}
	fs.writeFileSync(fname, data);
	console.log('wrote ' + fname);
}


function getFolder(name, cb) {
	//console.error('dir:'+name);

	let list = fs.readdirSync(name);
	for (let i = 0; i < list.length; i++) {
		let file = name + '/' + list[i];
		let s = fs.statSync(file);

		if (s.isDirectory()) {
			getFolder(file, cb);
		} else {
			let sourceFile = file.toLowerCase().endsWith('.c') || file.toLowerCase().endsWith('.cpp');
			let headerFile = file.toLowerCase().endsWith('.h');
			if (sourceFile || headerFile) {
				inFile++;
				//console.error('file:'+file);
				let data = fs.readFileSync(file);
				let text = data.toString('utf-8');
				let lines = text.split('\n');
				let newlines = [];
				let modified = 0;
				for (let i = 0; i < lines.length; i++) {
					let line = lines[i].trim();
					// like CMD_RegisterCommand("SetChannel", "", CMD_SetChannel, "qqqqq0", NULL);

					//parse enum starting with "typedef enum channelType_e {"
					/*
					if (headerFile && line.startsWith('typedef enum channelType_e {')) {
						newlines.push(lines[i]);
						let j;
						for (j = i; j < lines.length; j++) {
							let line2raw = lines[j];
							let line2 = line2raw.trim();
							if (line2.startsWith('//chandetail:')) {
								let commentlines = [];
								let j2;
								for (j2 = j; j2 < lines.length; j2++) {
									let l = lines[j2].trim();
									if (l.startsWith('//chandetail:')) {
										l = l.slice('//chandetail:'.length);
										commentlines.push(l);
										newlines.push(lines[j2]);
									} else {
										break;
									}
								}
								// move our parsing forward to skip all found
								j = j2;
								let json = commentlines.join('\n');
								try {
									let chan = JSON.parse(json);
									if (chanindex[chan.name]) {
										console.error('duplicate command docs at file: ' + file + ' line: ' + line);
										console.error(line);
									} else {
										channels.push(chan);
										console.log("Loading existing doc for " + chan.name);
										chanindex[chan.name] = chan;
									}
								} catch (e) {
									console.error('error in json at file: ' + file + ' line: ' + line + ' er ' + e);
									console.error(json);
								}
							} else if (line2.startsWith('//')) {
								newlines.push(line2);
								continue;
							} else if (line2.startsWith("ChType_")) {
								let chanName = line2.substr("ChType_".length);
								chanName = chanName.replace(/,$/, "");

								console.log("Channel type name: " + chanName);

								let chan = {
									name: chanName,
									title: chanName,
									descr: mytrim("TODO"),
									enum: "ChType_"+chanName,
									file: file.slice(6),
									driver: mytrim(""),
								};


								if (!chanindex[chan.name]) {
									console.log("Creating doc for " + chan.name);
									// it did not have a doc line before
									let json = JSON.stringify(chan);
									// insert CR at "fn":
									json = json.split('"title":');
									json = json.join('\n"title":');
									json = json.split('"descr":');
									json = json.join('\n"descr":');
									json = json.split('"enum":');
									json = json.join('\n"enum":');
									json = json.split('"file":');
									json = json.join('\n"file":');
									json = json.split('"driver":');
									json = json.join('\n"driver":');
									let jsonlines = json.split('\n');
									for (let j = 0; j < jsonlines.length; j++) {
										jsonlines[j] = '\t//chandetail:' + jsonlines[j];
									}
									newlines.push(...jsonlines);
									modified++;
									channels.push(chan);
									chanindex[chan.name] = chan;
								}
								newlines.push(lines[j]);
							}
							if (line2.endsWith('} channelType_t;')) {
								//newlines.push(line2raw);
								break;
							}
						}
						i = j;
					}
					*/
					if (headerFile && line.startsWith('typedef enum channelType_e {')) {
						newlines.push(lines[i]);
						let j;
						for (j = i; j < lines.length; j++) {
							let line2raw = lines[j];
							let line2 = line2raw.trim();
							if (line2.startsWith('//chandetail:')) {
								let commentlines = [];
								let j2;
								for (j2 = j; j2 < lines.length; j2++) {
									let l = lines[j2].trim();
									if (l.startsWith('//chandetail:')) {
										l = l.slice('//chandetail:'.length);
										commentlines.push(l);
										newlines.push(lines[j2]);
									} else {
										break;
									}
								}
								// move our parsing forward to skip all found
								// we are allready BEHIND comments when we used break, 
								// so we need to skip to j2-1 to handle the line in next loop 
								j = j2 - 1;
								let json = commentlines.join('\n');
								try {
									let chan = JSON.parse(json);
									if (chanindex[chan.name]) {
										console.error('duplicate channel docs at file: ' + file + ' line: ' + line);
										console.error(line);
									} else {
										channels.push(chan);
										//console.log("Loading existing doc for " + chan.name);
										chanindex[chan.name] = chan;
									}
								} catch (e) {
									console.error('error in json at file: ' + file + ' line: ' + line + ' er ' + e);
									console.error(json);
								}
							} else if (line2.startsWith('//')) {
								newlines.push(line2);
								continue;
							} else if (line2.startsWith("ChType_")) {
								let chanName = line2.substr("ChType_".length);
								chanName = chanName.replace(/,$/, "");

								console.log("Channel type name: " + chanName);

								let chan = {
									name: chanName,
									title: chanName,
									descr: mytrim("TODO"),
									enum: "ChType_" + chanName,
									file: file.slice(6),
									driver: mytrim(""),
								};


								if (!chanindex[chan.name]) {
									console.log("Creating doc for " + chan.name);
									// it did not have a doc line before
									let json = JSON.stringify(chan);
									// insert CR at "fn":
									json = json.split('"title":');
									json = json.join('\n"title":');
									json = json.split('"descr":');
									json = json.join('\n"descr":');
									json = json.split('"enum":');
									json = json.join('\n"enum":');
									json = json.split('"file":');
									json = json.join('\n"file":');
									json = json.split('"driver":');
									json = json.join('\n"driver":');
									let jsonlines = json.split('\n');
									for (let j = 0; j < jsonlines.length; j++) {
										jsonlines[j] = '\t//chandetail:' + jsonlines[j];
									}
									newlines.push(...jsonlines);
									modified++;
									channels.push(chan);
									chanindex[chan.name] = chan;
								}
								newlines.push(lines[j]);
							}
							if (line2.endsWith('} channelType_t;')) {
								//newlines.push(line2raw);
								break;
							}
						}
						i = j;
					}
					if (sourceFile && line.startsWith('const char* g_obk_flagNames[] = {')) {
						newlines.push(lines[i]);
						let j;
						for (j = i; j < lines.length; j++) {
							let line2raw = lines[j];
							let line2 = line2raw.trim();
							// console.log("line2 " + line2);
							if (line2.startsWith('//')) {
								newlines.push(line2);
								continue;
							} else if (line2.startsWith("\"[")) {
								let flagTitle = line2.trim().replace(/^"|"$/g, '');
								//   console.log("Old style flag title: " + flagTitle);
								if (flagTitle.endsWith('",')) {
									flagTitle = flagTitle.slice(0, -2);
								}
								if (flagTitle.endsWith(',')) {
									flagTitle = flagTitle.slice(0, -1);
								}
								oldStyleFlagTitles.push(flagTitle);

								newlines.push(lines[j]);
							}
							if (line2.endsWith('};')) {
								//newlines.push(line2raw);
								break;
							}
						}
						i = j;
					}
					if (headerFile && line.startsWith('#define')) {
						const parts = line.split(/[\s\t]+/);

						if (parts && parts.length > 2) {
							// The first part is the #define directive, which can be ignored
							const flagName = parts[1]; // OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER
							const flagValue = parts[2]; // 0
							// console.log("Define " + flagName);
							if (flagName.startsWith("OBK_FLAG_")) {

								let flag = {
									index: flagValue,
									enum: flagName,
									title: "todo",
									file: file.slice(6),
								};
								if (!flagindex[flag.index]) {
									// console.log("Creating flag " + flag.enum + " with index " + flag.index);
									flags.push(flag);
									flagindex[flag.index] = flag;
								}
							}
						}
					}
					// test for events (in cmd_eventHandlers.c)
					if (sourceFile && line.startsWith('int EVENT_ParseEventName')) {
						let j;
						let event;
						let foundif = "";
						for (j = i; j < lines.length; j++) {
							let line2raw = lines[j];
							let line2 = line2raw.trim();

							if (line2.endsWith('return CMD_EVENT_NONE;')) {
								break;
							}
							if (line2.startsWith("#if ")) {
								foundif = line2.replace(/^#if /, "");
							}
							if (line2.startsWith("#endif")) {
								foundif = "";
							}
							if (/if.*str[ni]*cmp/.test(line2)) {
								let Ename = line2.replace(/if.*str[ni]*cmp\(s,[ ]*"/, "").replace(/"[^"]*$/, '');
								if (Ename == 'channel') {
									Ename = Ename.replace("channel", "channel<X>"); // special case for channels and its number (...atoi ...)
								}
								let l = lines[++j].trim();
								i++;
								let CMD = l.replace(/^.*return /, "").replace(";", "");
								if (CMD.startsWith('CMD_EVENT_CHANGE_CHANNEL0')) {
									CMD = CMD.replace(/[ ]*\+[ ]*atoi.*/, '<X>'); // special case for channels and its number (...atoi ...)
								}
								let event = {
									name: Ename,
									CMD: CMD,
								};

								if (foundif != "") {
									event.ifdef = foundif;
								}
								evnts.push(event);
							}
						}
						i += j;
					}
					if (headerFile && line.startsWith('typedef enum ioRole_e {')) {
						newlines.push(lines[i]);
						let j;
						for (j = i; j < lines.length; j++) {
							let line2raw = lines[j];
							let line2 = line2raw.trim();
							if (line2.startsWith('//iodetail:')) {
								let commentlines = [];
								let j2;
								for (j2 = j; j2 < lines.length; j2++) {
									let l = lines[j2].trim();
									if (l.startsWith('//iodetail:')) {
										l = l.slice('//iodetail:'.length);
										commentlines.push(l);
										newlines.push(lines[j2]);
									} else {
										break;
									}
								}
								// move our parsing forward to skip all found
								// we are allready BEHIND comments when we used break, 
								// so we need to skip to j2-1 to handle the line in next loop 
								j = j2 - 1;
								let json = commentlines.join('\n');
								try {
									let io = JSON.parse(json);
									if (ioindex[io.name]) {
										console.error('duplicate io docs at file: ' + file + ' line: ' + line);
										console.error(line);
									} else {
										ios.push(io);
										ioindex[io.name] = io;
									}
								} catch (e) {
									console.error('error in json at file: ' + file + ' line: ' + line + ' er ' + e);
									console.error(json);
								}
							} else if (line2.startsWith('//')) {
								newlines.push(line2);
								continue;
							} else if (line2.startsWith("IOR_")) {
								let ioName = line2.substr("IOR_".length);
								ioName = ioName.replace(/,$/, "");

								console.log("ionel type name: " + ioName);

								let io = {
									name: ioName,
									title: ioName,
									descr: mytrim("TODO"),
									enum: "IOR_" + ioName,
									file: file.slice(6),
									driver: mytrim(""),
								};


								if (!ioindex[io.name]) {
									// it did not have a doc line before
									let json = JSON.stringify(io);
									// insert CR at "fn":
									json = json.split('"title":');
									json = json.join('\n"title":');
									json = json.split('"descr":');
									json = json.join('\n"descr":');
									json = json.split('"enum":');
									json = json.join('\n"enum":');
									json = json.split('"file":');
									json = json.join('\n"file":');
									json = json.split('"driver":');
									json = json.join('\n"driver":');
									let jsonlines = json.split('\n');
									for (let j = 0; j < jsonlines.length; j++) {
										jsonlines[j] = '\t//iodetail:' + jsonlines[j];
									}
									newlines.push(...jsonlines);
									modified++;
									ios.push(io);
									ioindex[io.name] = io;
								}
								newlines.push(lines[j]);
							}
							if (line2.endsWith('} ioRole_t;')) {
								//newlines.push(line2raw);
								break;
							}
						}
						i = j;
					}
					if (sourceFile && line.startsWith('static driver_t g_drivers[] = {')) {
						newlines.push(lines[i]);
						let j;
						let lasthash = "nothing yet";
						for (j = i; j < lines.length; j++) {
							let line2raw = lines[j];
							let line2 = line2raw.trim();
							if (line2.startsWith('#if')) {
								// try finding #if ENABLE_DRIVER_XY so we can use it in cas of a duplicate driver
								lasthash = line2;
							}
							if (line2.startsWith('//drvdetail:')) {
								let commentlines = [];
								let j2;
								for (j2 = j; j2 < lines.length; j2++) {
									let l = lines[j2].trim();
									if (l.startsWith('//drvdetail:')) {
										l = l.slice('//drvdetail:'.length);
										commentlines.push(l);
										newlines.push(lines[j2]);
									} else {
										break;
									}
								}
								// move our parsing forward to skip all found
								// we are allready BEHIND comments when we used break, 
								// so we need to skip to j2-1 to handle the line in next loop 
								j = j2 - 1;
								let json = commentlines.join('\n');
								try {
									let drv = JSON.parse(json);
									if (drvindex[drv.name]) {
										console.error('duplicate driver docs (in "' + line + '") for drv.name="' + drv.name + '" at file: ' + file + '  --  actual line:' + line2);
										console.error('\tlast "#if" statement: "' + lasthash + '"' + '\n\tfirst defined with "#if" statement: "' + drvdefines[drv.name] + '"');
										let tmpcmd = drvindex[drv.name];
										delete tmpcmd.define;		// we added "define" on the other element, but it's not present here
										if (JSON.stringify(tmpcmd) == JSON.stringify(drv)) console.error('\tshould be safe to ignore, because documentation is equal!');
										else console.error('\tFirst found:\n\t\t"' + JSON.stringify(tmpcmd).replace(/,\"/g, "\n\t\t\t\"") + '"\n\tactual:\n\t\t"' + JSON.stringify(drv).replace(/,\"/g, "\n\t\t\t\"") + '"');
										//console.error(line);
									} else {
										drv.define = 'Enabled by defining "' + lasthash.replace("defined(", "").replace(")", "").replace(/#if[^ ]* /, "<b>") + '</b>" for your platform in [obk_config.h](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/src/obk_config.h) ';
										drvs.push(drv);
										drvindex[drv.name] = drv;
										drvdefines[drv.name] = lasthash;
									}
								} catch (e) {
									console.error('error in json at file: ' + file + ' line: ' + line + ' er ' + e);
									console.error(json);
								}
							} else if (line2.startsWith('//')) {
								newlines.push(line2raw);
								continue;
							} else if (line2.startsWith('#')) {
								newlines.push(line2);
								continue;
							} else {
								//  console.log("line2 name: " + line2);
								const regex = /"([^"]+)"/; // This regular expression matches anything between quotes, but excludes the quotes themselves
								const startQuote = line2.indexOf('"') + 1; // Find the index of the first double-quote and add 1 to skip over it
								const endQuote = line2.indexOf('"', startQuote); // Find the index of the second double-quote starting from the first double-quote index
								let drvName = line2.substring(startQuote, endQuote); // Extract the substring between the first and second double-quotes

								if (endQuote != -1) {
									// drvName = drvName.replace(/,$/, "");

									console.log("drv type name: " + drvName);

									let drv = {
										name: drvName,
										title: drvName,
										descr: mytrim("TODO"),
										requires: mytrim(""),
									};


									if (!drvindex[drv.name]) {
										// it did not have a doc line before
										let json = JSON.stringify(drv);
										// insert CR at "fn":
										json = json.split('"title":');
										json = json.join('\n"title":');
										json = json.split('"descr":');
										json = json.join('\n"descr":');
										json = json.split('"requires":');
										json = json.join('\n"requires":');
										let jsonlines = json.split('\n');
										for (let j = 0; j < jsonlines.length; j++) {
											jsonlines[j] = '\t//drvdetail:' + jsonlines[j];
										}
										newlines.push(...jsonlines);
										modified++;
										drvs.push(drv);
										drvindex[drv.name] = drv;
									}
									newlines.push(lines[j]);
									// we found a driver definition before, so if its not a "one liner" we already copied before
									// we need to copy the next lines, until we find the closing "},"
									if (!lines[j].trim().endsWith('},')) {
										let j2;
										for (j2 = j + 1; j2 < lines.length; j2++) {
											let l = lines[j2].trim();
											if (l.endsWith('},')) {
												newlines.push(lines[j2]);
												break;
											}
											else {
												newlines.push(lines[j2]);
											}
										}
										// move our parsing forward to skip all found
										// arguments,  
										// so we need to skip to j2-1 to handle the line in next loop 
										j = j2;
									}



								}
							}
							if (line2.endsWith('};')) {
								//newlines.push(line2raw);
								break;
							}
						}
						i = j;
					}

					if (sourceFile && line.startsWith('const constant_t g_constants[] = {')) {
						newlines.push(lines[i]);
						let j;
						for (j = i; j < lines.length; j++) {
							let line2raw = lines[j];
							let line2 = line2raw.trim();
							if (line2.startsWith('//cnstdetail:')) {
								let commentlines = [];
								let j2;
								for (j2 = j; j2 < lines.length; j2++) {
									let l = lines[j2].trim();
									if (l.startsWith('//cnstdetail:')) {
										l = l.slice('//cnstdetail:'.length);
										commentlines.push(l);
										newlines.push(lines[j2]);
									} else {
										break;
									}
								}
								// move our parsing forward to skip all found
								// we are allready BEHIND comments when we used break, 
								// so we need to skip to j2-1 to handle the line in next loop 
								j = j2 - 1;
								let json = commentlines.join('\n');
								try {
									let cnst = JSON.parse(json);
									if (cnstindex[cnst.name]) {
										console.error('duplicate constant docs at file: ' + file + ' line: ' + line);
										console.error(line);
									} else {
										cnsts.push(cnst);
										cnstindex[cnst.name] = cnst;
									}
								} catch (e) {
									console.error('error in json at file: ' + file + ' line: ' + line + ' er ' + e);
									console.error(json);
								}
							} else if (line2.startsWith('//')) {
								newlines.push(line2);
								continue;
							} else if (line2.startsWith('#')) {
								newlines.push(line2);
								continue;
							} else {
								//  console.log("line2 name: " + line2);
								const regex = /"([^"]+)"/; // This regular expression matches anything between quotes, but excludes the quotes themselves
								const startQuote = line2.indexOf('"') + 1; // Find the index of the first double-quote and add 1 to skip over it
								const endQuote = line2.indexOf('"', startQuote); // Find the index of the second double-quote starting from the first double-quote index
								let cnstName = line2.substring(startQuote, endQuote); // Extract the substring between the first and second double-quotes

								if (endQuote != -1) {
									// cnstName = cnstName.replace(/,$/, "");

									console.log("cnst type name: " + cnstName);

									let cnst = {
										name: cnstName,
										title: cnstName,
										descr: mytrim("TODO"),
										requires: mytrim(""),
									};


									if (!cnstindex[cnst.name]) {
										// it did not have a doc line before
										let json = JSON.stringify(cnst);
										// insert CR at "fn":
										json = json.split('"title":');
										json = json.join('\n"title":');
										json = json.split('"descr":');
										json = json.join('\n"descr":');
										json = json.split('"requires":');
										json = json.join('\n"requires":');
										let jsonlines = json.split('\n');
										for (let j = 0; j < jsonlines.length; j++) {
											jsonlines[j] = '\t//cnstdetail:' + jsonlines[j];
										}
										newlines.push(...jsonlines);
										modified++;
										cnsts.push(cnst);
										cnstindex[cnst.name] = cnst;
									}
									newlines.push(lines[j]);
								}
							}
							if (line2.endsWith('};')) {
								//newlines.push(line2raw);
								break;
							}
						}
						i = j;
					}
					if (sourceFile && line.startsWith('//cmddetail:')) {
						let commentlines = [];
						let j;
						for (j = i; j < lines.length; j++) {
							let l = lines[j].trim();
							if (l.startsWith('//cmddetail:')) {
								l = l.slice('//cmddetail:'.length);
								commentlines.push(l);
								newlines.push(lines[j]);
							} else {
								break;
							}
						}
						// move our parsing forward to skip all found
						i = j;
						let json = commentlines.join('\n');
						try {
							let cmd = JSON.parse(json);
							if (cmdindex[cmd.name]) {
								console.error('duplicate command "' + cmd.name + '" docs at file: ' + file + ' line: ' + line + '\n\tfirst seen in "' + cmdindex[cmd.name].file + '"');
								tmp = cmdindex[cmd.name];	// to test, if oth are equal (despit different file) construct a helper ...
								tmp.file = cmd.file;	// ... and set its "file" to the actual value
								if (JSON.stringify(tmp) == JSON.stringify(cmd)) console.error('\tshould be safe to ignore, they are equal beside the file name!');
								else {
									console.error('\tFirst found:\n\t\t"' + JSON.stringify(cmdindex[cmd.name]).replace(/,\"/g, "\n\t\t\t\"") + '"\n\tactual:\n\t\t"' + JSON.stringify(cmd).replace(/,\"/g, "\n\t\t\t\""));
								}
							} {
								//console.error('new command "' + cmd.name + '" docs at file: ' + file + ' line: ' + line + ' -- json='+ json );
								if (cmd.file !== file.slice(6)) {
									console.error('!!!! Posible wrong file location for command "' + cmd.name + '": found in file: "' + file.slice(6) + '" but claimes file: "' + cmd.file + '" - please verify! !!!!')
									console.error('\t Posible fix: sed -i \'' + (i - 3) + ',' + (i - 1) + ' { /cmddetail:\\"fn\\":\\"' + cmd.fn + '\"/ s%' + cmd.file + "%" + file.slice(6) + '%} \'  src/' + file.slice(6))
									console.error('\t test posible fix: sed -n \'' + (i - 3) + ',' + (i - 1) + ' {/cmddetail:\\"fn\\":\\"' + cmd.fn + '\"/ s%' + cmd.file + "%" + file.slice(6) + '% p }\'  src/' + file.slice(6))

								}
								commands.push(cmd);
								cmdindex[cmd.name] = cmd;
							}
						} catch (e) {
							console.error('error in json at file: ' + file + ' line: ' + line + ' er ' + e);
							console.error(json);
						}
					}

					// i may have changed...
					line = lines[i].trim();

					if (sourceFile && line.startsWith('CMD_RegisterCommand(')) {
						line = line.slice('CMD_RegisterCommand('.length);
						parts = line.split(',');
						//cmddetail:{"name":"SetChannel", "args":"TODO", "fn":"CMD_SetChannel", "descr":"qqqqq0", "example":"", "file":"");
						//
						// CMD registration command line
						// CMD_RegisterCommand("SetChannel", CMD_SetChannel, NULL);


						let cmd = {
							name: mytrim(parts[0]),
							args: mytrim("TODO"),
							descr: mytrim(""),
							fn: mytrim(parts[1]),
							file: file.slice(6),
							requires: "",
							examples: "",
						};

						//if (cmd.descr !== 'NULL'){
						//    console.log('replace "'+cmd.descr+'" with NULL');
						//   lines[i] = lines[i].replace(', NULL, NULL);', ', NULL);');
						//   modified++;
						//}
						//
						// 20250908 only used once to fix broken auto-generated fn entries with "NULL);" entry
						//if (cmdindex[cmd.name] && cmdindex[cmd.name].fn == 'NULL);'){
						//    console.log('replace fn "'+cmdindex[cmd.name].fn+'" with ' + cmd.fn);
						//    console.log('newlines i-2 =' +newlines[i-2]);
						//    cmdindex[cmd.name].fn = cmd.fn;
						//    newlines[i-2] = newlines[i-2].replace('NULL);', cmd.fn);
						//    console.log('newlines i-2 =' +newlines[i-2]);
						//   modified++;
						//}

						if (cmdindex[cmd.name] && cmdindex[cmd.name].fn !== cmd.fn) {
							console.log('!!!!  "' + cmd.name + '" in file ' + cmd.file + ' -- fn "' + cmdindex[cmd.name].fn + '"  != called function "' + cmd.fn + '"  !!!!');
							console.log('\t possible fix: sed -i \'' + (i - 3) + ',' + (i - 1) + ' { /cmddetail:\\"fn\\":\\"' + cmdindex[cmd.name].fn + '\\"/ s%' + cmdindex[cmd.name].fn + "%" + cmd.fn + '% }\'  src/' + file.slice(6));
							console.log('\t test possible fix: sed -n \'' + (i - 3) + ',' + (i - 1) + ' { /cmddetail:\\"fn\\":\\"' + cmdindex[cmd.name].fn + '\\"/ s%' + cmdindex[cmd.name].fn + "%" + cmd.fn + '% p }\'  src/' + file.slice(6));
						}

						if (!cmdindex[cmd.name]) {
							// it did not have a doc line before
							let json = JSON.stringify(cmd);
							// insert CR at "fn":
							json = json.split('"descr":');
							json = json.join('\n"descr":');
							json = json.split('"fn":');
							json = json.join('\n"fn":');
							json = json.split('"examples":');
							json = json.join('\n"examples":');
							let jsonlines = json.split('\n');
							for (let j = 0; j < jsonlines.length; j++) {
								jsonlines[j] = '\t//cmddetail:' + jsonlines[j];
							}
							newlines.push(...jsonlines);
							modified++;
							commands.push(cmd);
							cmdindex[cmd.name] = cmd;
						}
					}

					newlines.push(lines[i]);
				}
				if (modified) {
					let newdata = newlines.join('\n');
					try {
						// write new file as "proposal", so we can choose, if/how to change source file
						writeIfChanged(file + ".getcommands", newdata);
					} catch (e) {
						console.error('failed to update ' + file);
					}
				}

				inFile--;

				if (!inFile) {
					//if (cb) cb();
				}
			}
		}
	}
}

// match flag titles


console.log('starting');

function removeGetCommandsFiles(dir) {
	let list = fs.readdirSync(dir);
	for (let i = 0; i < list.length; i++) {
		let filename = list[i];
		let file = dir + '/' + filename;
		let s = fs.statSync(file);
		if (s.isDirectory()) {
			removeGetCommandsFiles(file);
		} else {
			if (filename.endsWith('.getcommands')) {
				fs.unlinkSync(file);
				console.log('Removed ' + file);
			}
		}
	}
}

removeGetCommandsFiles('./src');

getFolder('./src');


function readJSONFile(path) {
	console.log("Going to read " + path);
	const data = fs.readFileSync(path, 'utf8');
	const jsonData = JSON.parse(data);
	return jsonData;
}

let faq = readJSONFile("docs/json/faq.json");
let generic = readJSONFile("docs/json/generic.json");
let commandExamples = readJSONFile("docs/json/commandExamples.json");
let autoexecExamples = readJSONFile("docs/json/autoexecExamples.json");
let scriptExamples = readJSONFile("docs/json/scriptExamples.json");
let mqttTopics = readJSONFile("docs/json/mqttTopics.json");
let subpages = readJSONFile("docs/json/subpages.json");

for (let i = 0; i < scriptExamples.length; i++) {
	let path = "docs/" + scriptExamples[i].file;
	const data = fs.readFileSync(path, 'utf8');
	scriptExamples[i].fileText = data;
}
for (let i = 0; i < autoexecExamples.length; i++) {
	let path = "docs/" + autoexecExamples[i].file;
	const data = fs.readFileSync(path, 'utf8');
	autoexecExamples[i].fileText = data;
}

let faqmdshort =
	`# FAQ (Frequently Asked Questions)
Here is the latest, up to date FAQ.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All questions/answers were taken from json file.

`;
let commandExamplesmdshort =
	`# Console command examples
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All command examples were taken from json file.

`;
let autoexecsmdshort =
	`# Example 'autoexec.bat' files


`;
let scriptsmdshort =
	`# Example script files


`;

let channelsmdshort =
	`# Channel Types
Here is the latest, up to date Channel Types.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Do not add anything here, as it will overwritten with next rebuild.
| ChannelType     |  Description  |
|:------------- | -------:|
`;

let iosmdshort =
	`# IO Pin Roles
Here is the latest, up to date IO Roles.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Do not add anything here, as it will overwritten with next rebuild.
| RoleName     |  Description  |
|:------------- | -------:|
`;


let flagsmdshort =
	`# Flags
Here is the latest, up to date Flags list.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Do not add anything here, as it will overwritten with next rebuild.
| ID |   Description  |
|:--| -------:|
`;
let driversmdshort =
	`# Drivers
Here is the latest, up to date drivers list.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Remember that some drivers might not be yet enabled on certain platforms,
but we can enable them for you per request. Some drivers might also be WIP.
Do not add anything here, as it will overwritten with next rebuild.
| Driver        | Description  |
|:------------- |:----- |
`;

let constantsmdshort =
	`# Constants (script variables)
Here is the latest, up to date constants list.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Constants can be accessed in commands, so things like setChannel 15 2*$CH14+5 can work.
Do not add anything here, as it will overwritten with next rebuild.
| Code        | Description  |
|:------------- | -----:|
`;
let mdshort =
	`# Commands
Here is the latest, up to date command list.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.<br><br>
Please remember that some commands might require starting a related driver first, so, for example, you must first do 'startDriver DoorSensor' and then 'DSEdge 1'.
Also remember that commands can be put in autoexec.bat to run at startup (see Web Application->LittleFS tab).
<br><br>
Do not add anything here, as it will overwritten with next rebuild.
| Command        | Arguments          | Description  |
|:------------- |:------------- |:----- |
`;

let mdlong =
	`# Commands
Here is the latest, up to date command list.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Do not add anything here, as it will overwritten with next rebuild.
| Command        | Arguments          | Description  | Location |
|:------------- |:-------------:|:----- | ------:|
`;

let mqttText =
	`# MQTT topics of published variables
Some MQTT variables are being published only at the startup, some are published periodically (if you enable "broadcast every N seconds" flag, default time is one minute, customizable with command mqtt_broadcastInterval), some are published only when a given value is changed. Below is the table of used publish topics (TODO: add full descriptions)

Hint: in HA, you can use MQTT wildcard to listen to multiple publishes. OBK_DEV_NAME/#

Publishes send by OBK device:
| Topic        | Sample Value          | Description  |
|:------------- |:------------- | -----:|
`;

let evntsmdshort =
	`# Events
Here is the latest, up to date documentation of possible events (Work in progress).
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Do not add anything here, as it will overwritten with next rebuild.
| Event     |  Eventcmd  |
|:------------- |:------- |
`;
for (let i = 0; i < evnts.length; i++) {
	let ev = evnts[i];


	let textshort = `| ${ev.name.replace("<", "&lt;").replace(">", "&gt;")} | ${ev.CMD.replace("<", "&lt;").replace(">", "&gt;")}${ev.ifdef ? '\nonly if defined:\n' + ev.ifdef.replace(/\|/g, "\\|") : ''} |`;
	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');
	evntsmdshort += textshort + '\n';
}

evntsmdshort += '\n';




function genReadMore(name) {
	let textmore = `See also [${name} on forum](https://www.elektroda.com/rtvforum/find.php?q=${name}).`;
	return textmore;
}
for (let i = 0; i < mqttTopics.publishes.length; i++) {
	let pub = mqttTopics.publishes[i];
	let textshort = `| ${pub.topic} | ${pub.example} | ${pub.description} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	mqttText += textshort;
	mqttText += '\n';
}

mqttText +=
	`

Publishes received by OBK device:
| Topic        | Sample Value          | Description  |
|:------------- |:------------- | -----:|
`;
let publishes_and_listens = mqttTopics.listens.concat(mqttTopics.publishes);
for (let i = 0; i < mqttTopics.listens.length; i++) {
	let pub = mqttTopics.listens[i];

	let textshort = `| ${pub.topic} | ${pub.example} | ${pub.description} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	mqttText += textshort;
	mqttText += '\n';
}



for (let i = 0; i < flags.length; i++) {
	let flag = flags[i];
	//console.log("Test flag " + i + " vs len " + oldStyleFlagTitles.length);
	if (i < oldStyleFlagTitles.length) {
		//console.log("Matched flag " + i);
		flag.descr = oldStyleFlagTitles[i];
	}
}

// Sort commands by Name alphabetically
commands.sort((a, b) => {
	if (a.name.toUpperCase() < b.name.toUpperCase())
		return -1;
	if (a.name.toUpperCase() > b.name.toUpperCase())
		return 1;
	return 0;
});
function formatDesc(descBasic) {
	if (!descBasic.endsWith('.')) {
		descBasic += '.';
	}
	if (descBasic.length > 0) {
		descBasic = descBasic.charAt(0).toUpperCase() + descBasic.slice(1);
	}
	return descBasic;
}
for (let i = 0; i < commands.length; i++) {
	/* like:
	| Command        | Arguments          | Description  |
	| ------------- |:-------------:| -----:|
	| setChannel     | [ChannelIndex][ChannelValue] | Sets a raw channel to given value. Relay channels are using 1 and 0 values. PWM channels are within [0,100] range. Do not use this for LED control, because there is a better and more advanced LED driver with dimming and configuration memory (remembers setting after on/off), LED driver commands has "led_" prefix. |
	| addChannel     | [ChannelIndex][ValueToAdd][ClampMin][ClampMax] | Ads a given value to the channel. Can be used to change PWM brightness. Clamp min and max arguments are optional. |
	*/
	let cmd = commands[i];


	let descMore = "<br/><br/>" + genReadMore(cmd.name);
	let descBasic = formatDesc(cmd.descr);
	let textshort = `| ${cmd.name} | ${cmd.args}${cmd.requires ? '\nReq:' + cmd.requires : ''} | ${descBasic}${cmd.examples ? '<br/><br/>Example: ' + cmd.examples : ''}${descMore} |`;
	let textlong = `| ${cmd.name} | ${cmd.args}${cmd.requires ? '\nReq:' + cmd.requires : ''} | ${descBasic}${cmd.examples ? '<br/><br/>Example: ' + cmd.examples : ''}${descMore} | File: ${cmd.file}\nFunction: ${cmd.fn} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');
	textlong = textlong.replace(/\n/g, '<br/>');

	mdshort += textshort;
	mdshort += '\n';
	mdlong += textlong;
	mdlong += '\n';
}

mdshort += '\n';
mdlong += '\n';


for (let i = 0; i < scriptExamples.length; i++) {
	let scr = scriptExamples[i];

	scriptsmdshort += "<b>" + scr.title + "</b>";
	scriptsmdshort += '\n';
	scriptsmdshort += '<br>';
	scriptsmdshort += "" + scr.description + "";
	scriptsmdshort += '\n';
	scriptsmdshort += '<br>';
	scriptsmdshort += 'Requirements:';
	scriptsmdshort += '\n';
	for (let j = 0; j < scr.requirements.length; j++) {
		scriptsmdshort += '- ' + scr.requirements[j] + '<br>';
		scriptsmdshort += '\n';
	}
	scriptsmdshort += '\n';
	scriptsmdshort += "```";
	scriptsmdshort += scr.fileText;
	scriptsmdshort += '\n';
	scriptsmdshort += "```";
	scriptsmdshort += '\n';

	scriptsmdshort += '\n';
	scriptsmdshort += '\n';
}
for (let i = 0; i < autoexecExamples.length; i++) {
	let exe = autoexecExamples[i];

	autoexecsmdshort += "" + exe.title + "";
	autoexecsmdshort += '\n';
	autoexecsmdshort += '<br>';
	autoexecsmdshort += '\n';
	autoexecsmdshort += "```c++";
	autoexecsmdshort += '\n';
	autoexecsmdshort += exe.fileText;
	autoexecsmdshort += '\n';
	autoexecsmdshort += "```";
	autoexecsmdshort += '\n';

	autoexecsmdshort += '\n';
	autoexecsmdshort += '\n';
}

for (let i = 0; i < faq.length; i++) {
	let q = faq[i];

	faqmdshort += "**Question:** *" + q.question + "*";
	faqmdshort += '<br>';
	faqmdshort += "**A:** " + q.answer;
	faqmdshort += '\n';

	faqmdshort += '\n';
	faqmdshort += '\n';
}

for (let i = 0; i < commandExamples.length; i++) {
	let ex = commandExamples[i];

	commandExamplesmdshort += "" + ex.description;
	commandExamplesmdshort += '\n';
	commandExamplesmdshort += '<br>';
	commandExamplesmdshort += "```" + ex.command + "```";
	commandExamplesmdshort += '<br>';
	commandExamplesmdshort += '\n';

	commandExamplesmdshort += '\n';
	commandExamplesmdshort += '\n';
	commandExamplesmdshort += '\n';
}

for (let i = 0; i < cnsts.length; i++) {
	let cn = cnsts[i];

	let textshort = `| ${cn.name} |  ${cn.descr} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	constantsmdshort += textshort;
	constantsmdshort += '\n';
}

for (let i = 0; i < drvs.length; i++) {
	let drv = drvs[i];

	let descMore = "<br/>" + genReadMore(drv.name);
	let descBasic = formatDesc(drv.descr);
	let textshort = `| ${drv.name} |  ${descBasic}\n${drv.define}(for Details see [here](https://www.elektroda.com/rtvforum/topic4033833.html)).${descMore} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	driversmdshort += textshort;
	driversmdshort += '\n';
}

for (let i = 0; i < channels.length; i++) {
	let chan = channels[i];

	let textshort = `| ${chan.name} |  ${chan.descr} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	channelsmdshort += textshort;
	channelsmdshort += '\n';
}

for (let i = 0; i < flags.length; i++) {
	let flag = flags[i];

	// let textshort = `| ${flag.index} | ${flag.enum} |  ${flag.descr} |`;
	let textshort = `| ${flag.index} | ${flag.descr} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	flagsmdshort += textshort;
	flagsmdshort += '\n';
}

for (let i = 0; i < ios.length; i++) {
	let io = ios[i];

	let textshort = `| ${io.name} | ${io.descr} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	iosmdshort += textshort;
	iosmdshort += '\n';
}

const dirPath = 'docs/json';
if (!fs.existsSync(dirPath)) {
	fs.mkdirSync(dirPath, { recursive: true });
}


let links = [];

function writeDocMD_Array(name, content, json, label, bWriteJSON, desc) {
	let fullName = "docs/" + name + ".md";

	writeIfChanged(fullName, content);

	if (json) {
		console.log('There are ' + json.length + " " + name);
	}
	if (json && bWriteJSON) {
		let jsonName = `${dirPath}/${name}.json`;
		writeIfChanged(jsonName, JSON.stringify(json, null, 2));
	}

	let link = {
		name: name,
		content: content,
		json: json,
		fullName: fullName,
		label: label,
		desc: desc
	};
	links.push(link);
}

function writeDocMD_Page(page) {
	let fullName = "docs/" + page.mdName + ".md";
	let pageText = "# " + page.topic + "\n" + page.content;
	writeIfChanged(fullName, pageText);

	page.fullMDPath = fullName;
}

writeDocMD_Array('ioRoles', iosmdshort, ios, "IO/Pin Roles", true, generic.pins);
writeDocMD_Array('flags', flagsmdshort, flags, "Flags", true, generic.flags);
writeDocMD_Array('drivers', driversmdshort, drvs, "Drivers", true, generic.drivers);
writeDocMD_Array('constants', constantsmdshort, cnsts, "Script constants", true, generic.constants);
writeDocMD_Array('channelTypes', channelsmdshort, channels, "Channel Types", true, generic.channels);
writeDocMD_Array('faq', faqmdshort, faq, "FAQ", false, generic.faq);
writeDocMD_Array('commands', mdshort, commands, "Console/Script commands", true, generic.commands);
writeDocMD_Array('commandExamples', commandExamplesmdshort, commandExamples, "Command Examples", false, generic.commandExamples);
writeDocMD_Array('autoexecExamples', autoexecsmdshort, autoexecExamples, "Autoexec.bat examples (configs)", false, generic.autoexecExamples);
writeDocMD_Array('mqttTopics', mqttText, publishes_and_listens, "MQTT Topics", false, generic.mqttTopics);
writeDocMD_Array('scriptExamples', scriptsmdshort, scriptExamples, "Script examples", false, generic.scriptExamples);
writeDocMD_Array('commands-extended', mdlong, commands, "Console/Script commands [Extended Edition]", false, "More details on commands.")
writeDocMD_Array('events', evntsmdshort, evnts, "Events", true, generic.events);

let links_md =
	`# Documentation
Here is the latest, up to date documentation.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
The descriptions were taken from code and from the JSON files, and then converted to MarkDown.
Do not add anything here, as it will overwritten with next rebuild.`


links_md += `\n`
links_md += `\n`
links_md += `# Basic doc pages \n`

for (let i = 0; i < subpages.length; i++) {
	let page = subpages[i];
	writeDocMD_Page(page);
	let shortDesc = page.shortDesc;
	let base = "https://github.com/openshwprojects/OpenBK7231T_App/blob/main/";
	let url = base + page.fullMDPath;
	let textshort = `- [${page.topic}](${url}) - ${shortDesc}`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	links_md += textshort;
	links_md += '\n';
}

links_md += `\n`
links_md += `# Doc tables `

links_md += `
| Section        | Comment        |
|:------------- |------:|
`;


for (let i = 0; i < links.length; i++) {
	let link = links[i];
	let base = "https://github.com/openshwprojects/OpenBK7231T_App/blob/main/";
	let url = base + link.fullName;
	let total = link.json.length;
	let desc = link.desc;
	let textshort = `| [${link.label}](${url}) (${total} total) | ${desc} |`;

	// allow multi-row entries in table entries.
	textshort = textshort.replace(/\n/g, '<br/>');

	links_md += textshort;
	links_md += '\n';
}


writeDocMD_Array('README', links_md);
