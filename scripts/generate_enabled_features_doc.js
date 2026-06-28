const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const WORKFLOW = path.join(ROOT, '.github', 'workflows', 'workflow.yaml');
const CONFIG = path.join(ROOT, 'src', 'obk_config.h');
const DRIVER_MAIN = path.join(ROOT, 'src', 'driver', 'drv_main.c');
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

const MCU_NAMES = {
  OpenBK7231T: 'BK7231T',
  OpenBK7231N: 'BK7231N',
  OpenXR809: 'XR809',
  OpenXR806: 'XR806',
  OpenXR806_DCDC: 'XR806',
  OpenXR872: 'XR872',
  OpenBL602: 'BL602',
  OpenBL602_ALT: 'BL602',
  OpenW800: 'W800',
  OpenW600: 'W600',
  OpenLN882H: 'LN882H',
  OpenTR6260: 'TR6260',
  OpenRTL8710A: 'RTL8710A',
  OpenRTL8710B: 'RTL8710B',
  OpenRTL87X0C: 'RTL87X0C',
  OpenRTL8720D: 'RTL8720D',
  OpenECR6600: 'ECR6600',
  OpenRDA5981: 'RDA5981',
  OpenRTL8721DA: 'RTL8721DA',
  OpenRTL8720E: 'RTL8720E',
  OpenLN8825: 'LN8825',
  OpenBL616: 'BL616',
  OpenGD32VW553: 'GD32VW553',
  OpenBK7231N_ALT: 'BK7231N',
  OpenBK7231T_ALT: 'BK7231T',
  OpenBK7231U: 'BK7231U',
  OpenBK7238: 'BK7238',
  OpenBK7252: 'BK7252',
  OpenBK7252N: 'BK7252N',
  OpenESP32: 'ESP32',
  OpenESP32C2: 'ESP32C2',
  OpenESP32C3: 'ESP32C3',
  OpenESP32C5: 'ESP32C5',
  OpenESP32C6: 'ESP32C6',
  OpenESP32C61: 'ESP32C61',
  OpenESP32S2: 'ESP32S2',
  OpenESP32S3: 'ESP32S3',
  OpenESP8266: 'ESP8266',
  OpenTXW81X: 'TXW81X',
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
];

// Some platform build files add driver defines outside src/obk_config.h.
const PLATFORM_EXTRA_DEFINES = {
  OpenBK7231U: ['ENABLE_DRIVER_MDNS'],
  OpenBK7252: ['ENABLE_DRIVER_MDNS'],
  OpenBK7252N: ['ENABLE_DRIVER_MDNS'],
  OpenTR6260: ['ENABLE_DRIVER_MDNS'],
  OpenRTL87X0C: ['ENABLE_DRIVER_MDNS', 'ENABLE_DRIVER_GAITEKAC'],
  OpenECR6600: ['ENABLE_DRIVER_MDNS'],
  OpenESP32: ['ENABLE_DRIVER_MDNS'],
  OpenESP32C2: ['ENABLE_DRIVER_MDNS'],
  OpenESP32C3: ['ENABLE_DRIVER_MDNS'],
  OpenESP32C5: ['ENABLE_DRIVER_MDNS'],
  OpenESP32C6: ['ENABLE_DRIVER_MDNS'],
  OpenESP32C61: ['ENABLE_DRIVER_MDNS'],
  OpenESP32S2: ['ENABLE_DRIVER_MDNS'],
  OpenESP32S3: ['ENABLE_DRIVER_MDNS'],
  OpenESP8266: ['ENABLE_DRIVER_MDNS'],
};

const DRIVER_GROUPS = [
  ['Power', new Set([
    'TESTPOWER', 'RN8209', 'BL0942', 'BL0942SPI', 'HLW8112SPI',
    'ChargingLimit', 'BL0937', 'CSE7761', 'CSE7766',
  ])],
  ['Light/LED', new Set([
    'PixelAnim', 'Drawers', 'PWMG', 'PT6523', 'TextScroller',
    'SM16703P', 'SM15155E', 'DDPSend', 'DDP', 'PWMToggler',
    'MAX72XX_Clock', 'SM2135', 'BP5758D', 'BP1658CJ', 'SM2235',
    'SSD1306', 'MAX72XX', 'HT16K33', 'TM1637', 'GN6932',
    'TM1638', 'HD2015', 'KP18058',
  ])],
  ['Sensors', new Set([
    'PIR', 'DS3231', 'ADCButton', 'BMP280', 'BMPI2C', 'CHT83XX',
    'MCP9808', 'ADCSmoother', 'SHT3X', 'SGP', 'AHT2X', 'DS1820',
    'DS1820_FULL', 'Battery', 'NEO6M', 'LTR_ALS', 'DoorSensor',
    'MAX6675', 'MAX31855',
  ])],
  ['IR/RF', new Set(['IR', 'RC', 'IR2', 'TinyIR_NEC'])],
  ['Network/Integrations', new Set([
    'TuyaMCU', 'tmSensor', 'Roomba', 'GirierMCU', 'ESPHomeAPI',
    'TCL', 'OpenWeatherMap', 'Widget', 'NTP', 'MDNS', 'SSDP',
    'DGR', 'Wemo', 'Hue', 'Bridge', 'UartTCP', 'GaitekAC',
    'mqttServer', 'TXWCAM',
  ])],
];

function read(file) {
  return fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n');
}

function parseWorkflowBuilds() {
  const lines = read(WORKFLOW).split('\n');
  const builds = [];
  let current = null;

  function pushCurrent() {
    if (current && current.platform && current.variant) {
      builds.push(current);
    }
  }

  for (const raw of lines) {
    const line = raw.replace(/\s+$/, '');
    const trimmed = line.trim();
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

  return builds.filter((build) => PLATFORM_DEFINES[build.platform]);
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
  const defines = new Map();
  for (const name of PLATFORM_DEFINES[build.platform]) {
    defines.set(name, 1);
  }
  const variant = build.platform === 'OpenXR806_DCDC' ? 9 : VARIANTS[build.variant] || 0;
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

function groupRegisteredDrivers(drivers) {
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
    groups.push(`Other: ${Array.from(remaining).join(', ')}`);
  }

  return groups.join('<br>') || '-';
}

function buildRow(build) {
  const defines = resolveDefines(build);
  const registeredDrivers = parseRegisteredDrivers(defines);
  const driverSummary = groupRegisteredDrivers(registeredDrivers);

  return {
    build: build.platform,
    variant: build.variant,
    mcu: MCU_NAMES[build.platform] || build.platform.replace(/^Open/, ''),
    defines,
    driverSummary: driverSummary || '-',
    registeredDrivers,
  };
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
  lines.push('# Enabled Features by Build');
  lines.push('');
  lines.push("This file is autogenerated from `.github/workflows/workflow.yaml`, `src/obk_config.h`, platform build defines, and `src/driver/drv_main.c`.");
  lines.push("It shows what the active GitHub Actions firmware build matrix asks each platform/variant to compile in, and which drivers are registered for that build.");
  lines.push("Platform SDK makefiles may still add extra defines outside the shared config; known platform driver defines are folded in here.");
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
    ['Build', 'Variant', 'MCU', ...FEATURE_COLUMNS.map(([label]) => label)],
    rows.map((row) => [
      row.build,
      row.variant,
      row.mcu,
      ...FEATURE_COLUMNS.map(([, define]) => yes(row.defines, define)),
    ])
  ));
  lines.push('');
  lines.push('## Driver Groups');
  lines.push('');
  lines.push(table(
    ['Build', 'Variant', 'Enabled driver groups'],
    rows.map((row) => [row.build, row.variant, row.driverSummary])
  ));
  lines.push('');
  lines.push('## Registered Drivers');
  lines.push('');
  lines.push('These are the driver names present in the compiled `drv_main.c` driver table after platform, variant, and known platform makefile defines are applied.');
  lines.push('');

  for (const row of rows) {
    lines.push(`<details><summary>${row.build} (${row.variant})</summary>`);
    lines.push('');
    if (row.registeredDrivers.length) {
      for (const name of row.registeredDrivers) {
        lines.push(`- \`${name}\``);
      }
    } else {
      lines.push('- No registered drivers.');
    }
    lines.push('');
    lines.push('</details>');
    lines.push('');
  }

  return `${lines.join('\n').replace(/[ \t]+$/gm, '')}\n`;
}

function main() {
  const rows = parseWorkflowBuilds().map(buildRow);
  const content = render(rows);

  if (process.argv.includes('--check')) {
    const existing = fs.existsSync(OUT) ? read(OUT) : '';
    if (existing !== content) {
      console.error('docs/enabledFeatures.md is not up to date. Run: npm run enabled-features');
      process.exit(1);
    }
    return;
  }

  fs.writeFileSync(OUT, content);
  console.log(`Wrote ${path.relative(ROOT, OUT)} for ${rows.length} build variants.`);
}

main();
