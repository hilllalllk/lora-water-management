import assert from 'node:assert/strict';
import { describe, it } from 'node:test';

import { parseHex, run } from '../src/cli/frame-tool.ts';

describe('frame tool', () => {
  it('builds the documented telemetry and command frames', () => {
    assert.equal(
      run(['telemetry', '--src', '0x1001', '--seq', '42', '--level', '1250', '--valve', '1']),
      '01 01 10 01 2A 07 01 02 04 E2 02 01 01 45 CB',
    );
    assert.equal(run(['command', '--seq', '201', '--cmd-id', '15', '--op', 'VANA_AC']), '01 10 00 01 C9 03 00 0F 01 8B 42');
  });

  it('decodes a command result', () => {
    const out = run(['decode', '01 11 10 01 2C 06 00 0F 00 02 01 01 96 3E']);
    assert.match(out, /type {4}CmdResult/);
    assert.match(out, /id=15 Ok state=\{"valve":1\}/);
  });

  it('decodes a heartbeat', () => {
    assert.match(run(['decode', '01 02 10 01 2D 09 00 00 0E 10 30 2E 31 2E 30 F7 11']), /uptime {2}3600 s, fw 0\.1\.0/);
  });

  it('accepts hex with 0x prefixes and commas', () => {
    assert.deepEqual(parseHex('0x01, 0xFF'), Uint8Array.of(1, 255));
  });
});
