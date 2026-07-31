const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const test = require('node:test');

const {
  buildRow,
  collectBuildManifest,
  isEnabled,
  loadManifestRows,
  parseWorkflowBuilds,
  prepareBuildManifest,
  render,
  resolveDefines,
} = require('./generate_enabled_features_doc');

function row(platform, variant) {
  return buildRow({ platform, variant });
}

test('parses every active firmware matrix build without duplicates', () => {
  const builds = parseWorkflowBuilds();
  const keys = new Set(builds.map((build) => `${build.platform}:${build.variant}`));

  assert.equal(builds.length, 59);
  assert.equal(keys.size, builds.length);
});

test('models conditional ESP-IDF mDNS build flags', () => {
  assert.equal(isEnabled(row('OpenESP32C2', '2M').defines, 'ENABLE_DRIVER_MDNS'), false);
  assert.equal(isEnabled(row('OpenESP32C3', '2M').defines, 'ENABLE_DRIVER_MDNS'), false);
  assert.equal(isEnabled(row('OpenESP32C3', '4M').defines, 'ENABLE_DRIVER_MDNS'), true);
  assert.equal(isEnabled(row('OpenESP32C6', '4M').defines, 'ENABLE_DRIVER_MDNS'), true);
  assert.equal(isEnabled(row('OpenESP32S2', '4M').defines, 'ENABLE_DRIVER_MDNS'), false);
});

test('includes platform and flag-only drivers', () => {
  assert.ok(row('OpenTXW81X', 'default').includedDrivers.includes('TXWCAM'));
  assert.ok(row('OpenW800', 'default').includedDrivers.includes('DHT'));
  assert.ok(row('OpenBK7231N', 'powerMetering').includedDrivers.includes('BL0939SPI'));
});

test('rejects unknown platforms and variants', () => {
  assert.throws(() => resolveDefines({ platform: 'OpenUnknown', variant: 'default' }), /Unknown platform/);
  assert.throws(() => resolveDefines({ platform: 'OpenBK7231N', variant: 'unknown' }), /Unknown variant/);
});

test('renders one non-duplicated included-driver section', () => {
  const rows = parseWorkflowBuilds().map(buildRow);
  const output = render(rows);

  assert.match(output, /## Included Drivers/);
  assert.doesNotMatch(output, /## Registered Drivers/);
  assert.doesNotMatch(output, /Other:/);
});

test('prepares and collects compiler-reported feature manifests', (t) => {
  const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'obk-features-'));
  t.after(() => fs.rmSync(tempDir, { recursive: true, force: true }));
  const probeFile = path.join(tempDir, 'probe.h');
  const logFile = path.join(tempDir, 'build.log');

  prepareBuildManifest(probeFile);
  const probe = fs.readFileSync(probeFile, 'utf8');
  assert.match(probe, /OBK_FEATURE_DEFINE ENABLE_DRIVER_DHT=/);
  assert.match(probe, /OBK_FEATURE_DEFINE PLATFORM_TXW81X=/);

  fs.writeFileSync(logFile, [
    'note: #pragma message: OBK_FEATURE_DEFINE OBK_VARIANT=0',
    'note: #pragma message: OBK_FEATURE_DEFINE PLATFORM_TXW81X=1',
    'note: #pragma message: OBK_FEATURE_DEFINE ENABLE_NTP=1',
  ].join('\n'));
  collectBuildManifest('OpenTXW81X', 'default', logFile, tempDir);

  const manifest = JSON.parse(fs.readFileSync(path.join(tempDir, 'OpenTXW81X-default.json'), 'utf8'));
  assert.deepEqual(manifest.defines, {
    OBK_VARIANT: 0,
    PLATFORM_TXW81X: 1,
    ENABLE_NTP: 1,
  });
});

test('requires and renders a compiler manifest for every matrix build', (t) => {
  const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'obk-feature-matrix-'));
  t.after(() => fs.rmSync(tempDir, { recursive: true, force: true }));

  for (const build of parseWorkflowBuilds()) {
    const model = buildRow(build);
    const manifest = {
      platform: build.platform,
      variant: build.variant,
      defines: Object.fromEntries(model.defines),
    };
    fs.writeFileSync(path.join(tempDir, `${build.platform}-${build.variant}.json`), JSON.stringify(manifest));
  }

  const rows = loadManifestRows(tempDir);
  assert.equal(rows.length, 59);
  assert.match(render(rows, true), /real compiler invocation/);

  fs.rmSync(path.join(tempDir, 'OpenTXW81X-default.json'));
  assert.throws(() => loadManifestRows(tempDir), /Missing compiler manifests: OpenTXW81X/);
});
