const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const WORKFLOW = path.join(ROOT, '.github', 'workflows', 'workflow.yaml');
const CONFIG = path.join(ROOT, 'src', 'obk_config.h');
const DRIVER_MAIN = path.join(ROOT, 'src', 'driver', 'drv_main.c');
const PROBE_HEADER = path.join(ROOT, 'src', 'driver', 'obk_feature_manifest_probe.h');
const OUT = path.join(ROOT, 'docs', 'enabledFeatures.md');

const VARIANTS = {
  default: 0,
  berry: 1,
  tuyaMCU: 2,
  powerMetering: 3,
  irRemoteESP: 4,
  sensors: 5,
  hlw8112: 6,
  battery: 7,
  btproxy: 8,
  '2M': 1,
  '4M': 2,
  '2M_berry': 3,
};

const PLATFORM_DEFINES = {
  OpenBK7231T: ['PLATFORM_BEKEN', 'PLATFORM_BK7231T'],
  OpenBK7231N: ['PLATFORM_BEKEN', 'PLATFORM_BK7231N'],
  OpenXR809: ['PLATFORM_XRADIO', 'PLATFORM_XR809'],
  OpenXR806: ['PLATFORM_XRADIO', 'PLATFORM_XR806'],
  OpenXR806_DCDC: ['PLATFORM_XRADIO', 'PLATFORM_XR806'],
  OpenXR872: ['PLATFORM_XRADIO', 'PLATFORM_XR872'],
  OpenBL602: ['PLATFORM_BL602'],
  OpenBL602_ALT: ['PLATFORM_BL602', 'PLATFORM_BL_NEW'],
  OpenW800: ['PLATFORM_W800'],
  OpenW600: ['PLATFORM_W600'],
  OpenLN882H: ['PLATFORM_LN882H'],
  OpenTR6260: ['PLATFORM_TR6260'],
  OpenRTL8710A: ['PLATFORM_REALTEK', 'PLATFORM_RTL8710A'],
  OpenRTL8710B: ['PLATFORM_REALTEK', 'PLATFORM_RTL8710B'],
  OpenRTL87X0C: ['PLATFORM_REALTEK', 'PLATFORM_RTL87X0C'],
  OpenRTL8720D: ['PLATFORM_REALTEK', 'PLATFORM_RTL8720D'],
  OpenECR6600: ['PLATFORM_ECR6600'],
  OpenRDA5981: ['PLATFORM_RDA5981'],
  OpenRTL8721DA: ['PLATFORM_REALTEK', 'PLATFORM_RTL8721DA', 'PLATFORM_REALTEK_NEW'],
  OpenRTL8720E: ['PLATFORM_REALTEK', 'PLATFORM_RTL8720E', 'PLATFORM_REALTEK_NEW'],
  OpenLN8825: ['PLATFORM_LN8825'],
  OpenBL616: ['PLATFORM_BL616', 'PLATFORM_BL_NEW'],
  OpenGD32VW553: ['PLATFORM_GD32VW553'],
  OpenBK7231N_ALT: ['PLATFORM_BEKEN', 'PLATFORM_BEKEN_NEW', 'PLATFORM_BK7231N'],
  OpenBK7231T_ALT: ['PLATFORM_BEKEN', 'PLATFORM_BEKEN_NEW', 'PLATFORM_BK7231T'],
  OpenBK7231U: ['PLATFORM_BEKEN', 'PLATFORM_BEKEN_NEW', 'PLATFORM_BK7231U'],
  OpenBK7238: ['PLATFORM_BEKEN', 'PLATFORM_BEKEN_NEW', 'PLATFORM_BK7238'],
  OpenBK7252: ['PLATFORM_BEKEN', 'PLATFORM_BEKEN_NEW', 'PLATFORM_BK7252'],
  OpenBK7252N: ['PLATFORM_BEKEN', 'PLATFORM_BEKEN_NEW', 'PLATFORM_BK7252N'],
  OpenESP32: ['PLATFORM_ESPIDF', 'CONFIG_IDF_TARGET_ESP32'],
  OpenESP32C2: ['PLATFORM_ESPIDF', 'CONFIG_IDF_TARGET_ESP32C2'],
  OpenESP32C3: ['PLATFORM_ESPIDF', 'CONFIG_IDF_TARGET_ESP32C3'],
  OpenESP32C5: ['PLATFORM_ESPIDF', 'CONFIG_IDF_TARGET_ESP32C5'],
  OpenESP32C6: ['PLATFORM_ESPIDF', 'CONFIG_IDF_TARGET_ESP32C6'],
  OpenESP32C61: ['PLATFORM_ESPIDF', 'CONFIG_IDF_TARGET_ESP32C61'],
  OpenESP32S2: ['PLATFORM_ESPIDF', 'CONFIG_IDF_TARGET_ESP32S2'],
  OpenESP32S3: ['PLATFORM_ESPIDF', 'CONFIG_IDF_TARGET_ESP32S3'],
  OpenESP8266: ['PLATFORM_ESP8266'],
  OpenTXW81X: ['PLATFORM_TXW81X'],
};

const FEATURE_COLUMNS = [
  ['MQTT', 'ENABLE_MQTT'],
  ['HA', 'ENABLE_HA_DISCOVERY'],
  ['Tasmota', 'ENABLE_TASMOTA_JSON'],
  ['DGR', 'ENABLE_TASMOTADEVICEGROUPS'],
  ['NTP', 'ENABLE_NTP'],
  ['LFS', 'ENABLE_LITTLEFS'],
  ['Script', 'ENABLE_OBK_SCRIPTING'],
  ['Berry', 'ENABLE_OBK_BERRY'],
  ['BT Proxy', 'ENABLE_BT_PROXY'],
];

// Some platform build files add driver defines outside src/obk_config.h.
const PLATFORM_EXTRA_DEFINES = {
  OpenBK7231U: ['ENABLE_DRIVER_MDNS'],
  OpenBK7252: ['ENABLE_DRIVER_MDNS'],
  OpenBK7252N: ['ENABLE_DRIVER_MDNS'],
  OpenTR6260: ['ENABLE_DRIVER_MDNS'],
  OpenRTL87X0C: ['ENABLE_DRIVER_MDNS', 'ENABLE_DRIVER_GAITEKAC'],
  OpenECR6600: ['ENABLE_DRIVER_MDNS'],
  OpenESP8266: ['ENABLE_DRIVER_MDNS'],
};

const BUILD_JOBS = new Set(['build', 'build_idf', 'build_txw81x']);

// These compile-time drivers run outside the startDriver registration table.
const FLAG_ONLY_DRIVERS = new Map([
  ['ENABLE_DRIVER_DHT', 'DHT'],
]);

const DRIVER_GROUPS = [
  ['Power', new Set([
    'TESTPOWER', 'RN8209', 'BL0942', 'BL0942SPI', 'BL0939SPI', 'HLW8112SPI',
    'ChargingLimit', 'BL0937', 'CSE7761', 'CSE7766',
  ])],
  ['Light/LED', new Set([
    'PixelAnim', 'Drawers', 'PWMG', 'PT6523', 'TextScroller',
    'SM16703P', 'SM15155E', 'DDPSend', 'DDP', 'PWMToggler',
    'MAX72XX_Clock', 'SM2135', 'BP5758D', 'BP1658CJ', 'SM2235',
    'SSD1306', 'MAX72XX', 'HT16K33', 'TM1637', 'GN6932',
    'TM1638', 'HD2015', 'KP18058', 'DMX',
  ])],
  ['Sensors', new Set([
    'PIR', 'DS3231', 'ADCButton', 'BMP280', 'BMPI2C', 'CHT83XX',
    'MCP9808', 'ADCSmoother', 'SHT3X', 'SGP', 'AHT2X', 'DS1820',
    'DS1820_FULL', 'Battery', 'NEO6M', 'LTR_ALS', 'DoorSensor',
    'MAX6675', 'MAX31855', 'DHT',
  ])],
  ['IR/RF', new Set(['IR', 'RC', 'IR2', 'TinyIR_NEC'])],
  ['Network/Integrations', new Set([
    'TuyaMCU', 'tmSensor', 'Roomba', 'GirierMCU', 'ESPHomeAPI',
    'TCL', 'OpenWeatherMap', 'NTP', 'MDNS', 'SSDP',
    'DGR', 'Wemo', 'Hue', 'Bridge', 'UartTCP', 'GaitekAC',
    'mqttServer', 'TXWCAM',
  ])],
  ['Automation/UI', new Set(['Widget', 'HTTPButtons', 'Shutters', 'Charts'])],
  ['Interfaces', new Set(['I2C', 'TCA9554'])],
  ['Development', new Set(['Test'])],
];

function read(file) {
  return fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n');
}

function writeGeneratedFile(file, content) {
  const existing = fs.existsSync(file) ? fs.readFileSync(file, 'utf8') : '';
  const eol = existing.includes('\r\n') ? '\r\n' : '\n';
  fs.writeFileSync(file, content.replace(/\n/g, eol));
}

function parseWorkflowBuilds() {
  const lines = read(WORKFLOW).split('\n');
  const builds = [];
  let current = null;
  let currentJob = null;

  function pushCurrent() {
    if (current && current.platform && current.variant) {
      builds.push(current);
    }
  }

  for (const raw of lines) {
    const line = raw.replace(/\s+$/, '');
    const trimmed = line.trim();
    const jobMatch = line.match(/^  ([A-Za-z0-9_]+):\s*$/);
    if (jobMatch) {
      pushCurrent();
      current = null;
      currentJob = jobMatch[1];
      continue;
    }

    if (!BUILD_JOBS.has(currentJob)) {
      continue;
    }

    if (!trimmed || trimmed.startsWith('#')) {
      continue;
    }

    const platformMatch = line.match(/^\s*-\s+platform:\s*([^#\s]+)/);
    if (platformMatch) {
      pushCurrent();
      current = { platform: platformMatch[1] };
      continue;
    }

    if (!current) {
      continue;
    }

    const propMatch = line.match(/^\s+([A-Za-z0-9_]+):\s*([^#]+?)\s*$/);
    if (propMatch) {
      current[propMatch[1]] = propMatch[2].replace(/^['"]|['"]$/g, '');
    }
  }
  pushCurrent();

  const seen = new Set();
  for (const build of builds) {
    if (!PLATFORM_DEFINES[build.platform]) {
      throw new Error(`Unknown workflow platform: ${build.platform}`);
    }
    if (!Object.prototype.hasOwnProperty.call(VARIANTS, build.variant)) {
      throw new Error(`Unknown workflow variant: ${build.platform} (${build.variant})`);
    }
    const key = `${build.platform}:${build.variant}`;
    if (seen.has(key)) {
      throw new Error(`Duplicate workflow build: ${build.platform} (${build.variant})`);
    }
    seen.add(key);
  }

  return builds;
}

function asNumber(value) {
  if (value === undefined) {
    return 0;
  }
  if (typeof value === 'number') {
    return value;
  }
  const text = String(value).trim();
  if (/^-?\d+$/.test(text)) {
    return Number(text);
  }
  return text.length ? 1 : 0;
}

function evalCondition(expr, defines) {
  let js = expr
    .replace(/\/\*.*?\*\//g, ' ')
    .replace(/\/\/.*$/g, ' ')
    .replace(/\bdefined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)/g, (_, name) => (defines.has(name) ? '1' : '0'))
    .replace(/\bdefined\s+([A-Za-z_][A-Za-z0-9_]*)/g, (_, name) => (defines.has(name) ? '1' : '0'))
    .replace(/\b[A-Za-z_][A-Za-z0-9_]*\b/g, (name) => String(asNumber(defines.get(name))));

  if (!/^[\d\s!<>=&|()+\-*/%]+$/.test(js)) {
    throw new Error(`Unsupported preprocessor expression: ${expr}`);
  }
  return Function(`"use strict"; return Number(${js}) ? 1 : 0;`)();
}

function parseDefine(line) {
  const match = line.match(/^#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)(?:\s+(.*?))?\s*$/);
  if (!match) {
    return null;
  }
  let value = match[2] === undefined ? '1' : match[2].trim();
  value = value.replace(/\/\*.*?\*\//g, '').replace(/\/\/.*$/g, '').trim();
  if (!value) {
    value = '1';
  }
  if (/^-?\d+$/.test(value)) {
    value = Number(value);
  }
  return { name: match[1], value };
}

function walkPreprocessorFile(file, defines, onActiveLine) {
  const stack = [{ parentActive: true, active: true, branchTaken: false }];
  const lines = read(file).split('\n');

  function active() {
    return stack[stack.length - 1].active;
  }

  for (const raw of lines) {
    const line = raw.trim();
    if (!line.startsWith('#')) {
      if (active()) {
        onActiveLine(raw, line);
      }
      continue;
    }

    let match = line.match(/^#\s*if\s+(.+)$/);
    if (match) {
      const parentActive = active();
      const cond = parentActive ? evalCondition(match[1], defines) : 0;
      stack.push({ parentActive, active: !!(parentActive && cond), branchTaken: !!(parentActive && cond) });
      continue;
    }

    match = line.match(/^#\s*ifdef\s+([A-Za-z_][A-Za-z0-9_]*)/);
    if (match) {
      const parentActive = active();
      const cond = parentActive && defines.has(match[1]);
      stack.push({ parentActive, active: !!cond, branchTaken: !!cond });
      continue;
    }

    match = line.match(/^#\s*ifndef\s+([A-Za-z_][A-Za-z0-9_]*)/);
    if (match) {
      const parentActive = active();
      const cond = parentActive && !defines.has(match[1]);
      stack.push({ parentActive, active: !!cond, branchTaken: !!cond });
      continue;
    }

    match = line.match(/^#\s*elif\s+(.+)$/);
    if (match) {
      const frame = stack[stack.length - 1];
      const canTake = frame.parentActive && !frame.branchTaken;
      const cond = canTake ? evalCondition(match[1], defines) : 0;
      frame.active = !!(canTake && cond);
      frame.branchTaken = frame.branchTaken || !!(canTake && cond);
      continue;
    }

    if (/^#\s*else\b/.test(line)) {
      const frame = stack[stack.length - 1];
      frame.active = !!(frame.parentActive && !frame.branchTaken);
      frame.branchTaken = frame.branchTaken || frame.parentActive;
      continue;
    }

    if (/^#\s*endif\b/.test(line)) {
      if (stack.length === 1) {
        throw new Error('Unbalanced #endif in obk_config.h');
      }
      stack.pop();
      continue;
    }

    if (!active()) {
      continue;
    }

    onActiveLine(raw, line);
  }

  if (stack.length !== 1) {
    throw new Error(`Unbalanced preprocessor conditions in ${path.relative(ROOT, file)}`);
  }
}

function resolveDefines(build) {
  if (!PLATFORM_DEFINES[build.platform]) {
    throw new Error(`Unknown platform: ${build.platform}`);
  }
  if (!Object.prototype.hasOwnProperty.call(VARIANTS, build.variant)) {
    throw new Error(`Unknown variant: ${build.platform} (${build.variant})`);
  }

  const defines = new Map();
  for (const name of PLATFORM_DEFINES[build.platform]) {
    defines.set(name, 1);
  }
  const variant = build.platform === 'OpenXR806_DCDC' ? 9 : VARIANTS[build.variant];
  defines.set('OBK_VARIANT', variant);

  walkPreprocessorFile(CONFIG, defines, (raw, line) => {
    const define = parseDefine(line);
    if (define) {
      defines.set(define.name, define.value);
      return;
    }

    const match = line.match(/^#\s*undef\s+([A-Za-z_][A-Za-z0-9_]*)/);
    if (match) {
      defines.delete(match[1]);
    }
  });

  for (const name of PLATFORM_EXTRA_DEFINES[build.platform] || []) {
    defines.set(name, 1);
  }

  const espIdfMdnsPlatforms = new Set(['OpenESP32', 'OpenESP32C6', 'OpenESP32S3']);
  if (espIdfMdnsPlatforms.has(build.platform) ||
      (build.platform === 'OpenESP32C3' && build.variant === '4M')) {
    defines.set('ENABLE_DRIVER_MDNS', 1);
  }

  return defines;
}

function isEnabled(defines, name) {
  return asNumber(defines.get(name)) !== 0;
}

function enabledNames(defines, names) {
  return names.filter((name) => isEnabled(defines, name)).map(prettyName);
}

function prettyName(name) {
  return name
    .replace(/^ENABLE_DRIVER_/, '')
    .replace(/^ENABLE_/, '')
    .replace(/_/g, ' ');
}

function yes(defines, name) {
  return isEnabled(defines, name) ? '✅' : '-';
}

function parseRegisteredDrivers(defines) {
  const drivers = [];
  walkPreprocessorFile(DRIVER_MAIN, defines, (raw) => {
    const match = raw.match(/^\s*\{\s*"([^"]+)"/);
    if (match) {
      drivers.push(match[1]);
    }
  });
  return drivers;
}

function parseFlagOnlyDrivers(defines) {
  const driverMain = read(DRIVER_MAIN);
  const drivers = [];

  for (const name of defines.keys()) {
    if (name.startsWith('ENABLE_DRIVER_') && isEnabled(defines, name) && !driverMain.includes(name)) {
      const driver = FLAG_ONLY_DRIVERS.get(name);
      if (!driver) {
        throw new Error(`Unmapped enabled driver flag outside drv_main.c: ${name}`);
      }
      drivers.push(driver);
    }
  }

  return drivers;
}

function groupIncludedDrivers(drivers) {
  const remaining = new Set(drivers);
  const groups = [];

  for (const [group, knownDrivers] of DRIVER_GROUPS) {
    const enabled = drivers.filter((driver) => knownDrivers.has(driver));
    for (const driver of enabled) {
      remaining.delete(driver);
    }
    if (enabled.length) {
      groups.push(`${group}: ${enabled.join(', ')}`);
    }
  }

  if (remaining.size) {
    throw new Error(`Uncategorized enabled drivers: ${Array.from(remaining).join(', ')}`);
  }

  return groups.join('<br>') || '-';
}

function buildRow(build, compilerDefines) {
  const defines = compilerDefines instanceof Map ? compilerDefines : resolveDefines(build);
  const registeredDrivers = parseRegisteredDrivers(defines);
  const flagOnlyDrivers = parseFlagOnlyDrivers(defines);
  const includedDrivers = [...registeredDrivers, ...flagOnlyDrivers];
  const driverSummary = groupIncludedDrivers(includedDrivers);

  return {
    build: build.platform,
    variant: build.variant,
    defines,
    driverSummary: driverSummary || '-',
    registeredDrivers,
    flagOnlyDrivers,
    includedDrivers,
  };
}

function probeDefineNames() {
  const names = new Set(['OBK_VARIANT']);
  const source = `${read(CONFIG)}\n${read(DRIVER_MAIN)}`;
  for (const match of source.matchAll(/\b(?:CONFIG_IDF_TARGET_[A-Z0-9_]+|ENABLE_[A-Z0-9_]+|PLATFORM_[A-Z0-9_]+|WINDOWS)\b/g)) {
    names.add(match[0]);
  }
  return Array.from(names).sort();
}

function prepareBuildManifest(outputFile = PROBE_HEADER) {
  const names = probeDefineNames();
  const lines = [
    '// Autogenerated temporarily by CI. Do not commit generated probe contents.',
    '#define OBK_FEATURE_STRINGIFY_INNER(value) #value',
    '#define OBK_FEATURE_STRINGIFY(value) OBK_FEATURE_STRINGIFY_INNER(value)',
  ];
  for (const name of names) {
    lines.push(`#ifdef ${name}`);
    lines.push(`#pragma message("OBK_FEATURE_DEFINE ${name}=" OBK_FEATURE_STRINGIFY(${name}))`);
    lines.push('#endif');
  }
  fs.writeFileSync(outputFile, `${lines.join('\n')}\n`);
  console.log(`Prepared compiler probes for ${names.length} feature defines.`);
}

function collectBuildManifest(platform, variant, logFile, outputDir) {
  if (!PLATFORM_DEFINES[platform]) {
    throw new Error(`Unknown platform: ${platform}`);
  }
  if (!Object.prototype.hasOwnProperty.call(VARIANTS, variant)) {
    throw new Error(`Unknown variant: ${platform} (${variant})`);
  }

  const defines = {};
  const log = read(path.resolve(ROOT, logFile));
  const pattern = /OBK_FEATURE_DEFINE\s+([A-Z0-9_]+)=(-?\d+|[A-Za-z_][A-Za-z0-9_]*)/g;
  for (const match of log.matchAll(pattern)) {
    defines[match[1]] = /^-?\d+$/.test(match[2]) ? Number(match[2]) : match[2];
  }
  if (!Object.prototype.hasOwnProperty.call(defines, 'OBK_VARIANT')) {
    throw new Error(`Compiler feature probes were not found in ${logFile}`);
  }
  const hasPlatform = Object.keys(defines).some((name) => name.startsWith('PLATFORM_') || name.startsWith('CONFIG_IDF_TARGET_'));
  if (!hasPlatform) {
    throw new Error(`No platform define was reported for ${platform} (${variant})`);
  }

  const outDir = path.resolve(ROOT, outputDir);
  fs.mkdirSync(outDir, { recursive: true });
  const outFile = path.join(outDir, `${platform}-${variant}.json`);
  fs.writeFileSync(outFile, `${JSON.stringify({ platform, variant, defines }, null, 2)}\n`);
  console.log(`Collected ${Object.keys(defines).length} compiler defines in ${path.relative(ROOT, outFile)}.`);
}

function loadManifestRows(manifestDir) {
  const expected = parseWorkflowBuilds();
  const expectedKeys = new Set(expected.map((build) => `${build.platform}\0${build.variant}`));
  const manifests = new Map();

  for (const name of fs.readdirSync(manifestDir).filter((file) => file.endsWith('.json'))) {
    const manifest = JSON.parse(fs.readFileSync(path.join(manifestDir, name), 'utf8'));
    const key = `${manifest.platform}\0${manifest.variant}`;
    if (!expectedKeys.has(key)) {
      throw new Error(`Unexpected compiler manifest: ${manifest.platform} (${manifest.variant})`);
    }
    if (manifests.has(key)) {
      throw new Error(`Duplicate compiler manifest: ${manifest.platform} (${manifest.variant})`);
    }
    manifests.set(key, manifest);
  }

  const missing = expected.filter((build) => !manifests.has(`${build.platform}\0${build.variant}`));
  if (missing.length) {
    throw new Error(`Missing compiler manifests: ${missing.map((build) => `${build.platform} (${build.variant})`).join(', ')}`);
  }

  return expected.map((build) => {
    const manifest = manifests.get(`${build.platform}\0${build.variant}`);
    return buildRow(build, new Map(Object.entries(manifest.defines)));
  });
}

function table(headers, rows) {
  const out = [];
  out.push(`| ${headers.join(' | ')} |`);
  out.push(`| ${headers.map(() => '---').join(' | ')} |`);
  for (const row of rows) {
    out.push(`| ${row.join(' | ')} |`);
  }
  return out.join('\n');
}

function render(rows) {
  const lines = [];
  lines.push('# Enabled Features and Drivers by Build');
  lines.push('');
  lines.push("This document is autogenerated for the active GitHub Actions firmware build matrix; release builds use feature defines reported by each platform's real compiler invocation, while local regeneration provides a static preview from repository definitions.");
  lines.push("Driver lists combine names registered in `src/driver/drv_main.c` with enabled `ENABLE_DRIVER_*` capabilities implemented outside that table.");
  lines.push("The release artifact is rendered from compiler-reported manifests and fails if any active build is missing.");
  lines.push('');
  lines.push('Regenerate with:');
  lines.push('');
  lines.push('```sh');
  lines.push('npm run enabled-features');
  lines.push('```');
  lines.push('');
  lines.push('## Summary');
  lines.push('');
  lines.push(table(
    ['Build', 'Variant', ...FEATURE_COLUMNS.map(([label]) => label)],
    rows.map((row) => [
      row.build,
      row.variant,
      ...FEATURE_COLUMNS.map(([, define]) => yes(row.defines, define)),
    ])
  ));
  lines.push('');
  lines.push('HA = Home Assistant discovery; DGR = Tasmota Device Groups; LFS = LittleFS.');
  lines.push('');
  lines.push('## Included Drivers');
  lines.push('');
  lines.push(table(
    ['Build', 'Variant', 'Drivers and integrations'],
    rows.map((row) => [row.build, row.variant, row.driverSummary])
  ));

  return `${lines.join('\n').replace(/[ \t]+$/gm, '')}\n`;
}

function main() {
  const args = process.argv.slice(2);
  if (args[0] === '--prepare-build-manifest') {
    prepareBuildManifest();
    return;
  }
  if (args[0] === '--collect-build-manifest') {
    if (args.length !== 5) {
      throw new Error('Usage: --collect-build-manifest <platform> <variant> <build-log> <output-dir>');
    }
    collectBuildManifest(args[1], args[2], args[3], args[4]);
    return;
  }

  const manifestIndex = args.indexOf('--manifest-dir');
  if (manifestIndex !== -1 && !args[manifestIndex + 1]) {
    throw new Error('--manifest-dir requires a directory');
  }
  const compilerDerived = manifestIndex !== -1;
  const rows = compilerDerived
    ? loadManifestRows(path.resolve(ROOT, args[manifestIndex + 1]))
    : parseWorkflowBuilds().map(buildRow);
  const content = render(rows);

  if (process.argv.includes('--check')) {
    const existing = fs.existsSync(OUT) ? read(OUT) : '';
    if (existing !== content) {
      console.error('docs/enabledFeatures.md is not up to date. Run: npm run enabled-features');
      process.exit(1);
    }
    return;
  }

  writeGeneratedFile(OUT, content);
  console.log(`Wrote ${path.relative(ROOT, OUT)} for ${rows.length} build variants.`);
}

if (require.main === module) {
  main();
}

module.exports = {
  buildRow,
  collectBuildManifest,
  isEnabled,
  loadManifestRows,
  parseWorkflowBuilds,
  prepareBuildManifest,
  render,
  resolveDefines,
};
