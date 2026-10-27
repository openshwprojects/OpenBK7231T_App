#!/usr/bin/env python3
"""Run real Hue/Wemo driver code with host hardware/HTTP stubs. Needs gcc and Python 3."""
import ctypes as c
import json
import socket
import threading
import time
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
TEMP = tempfile.TemporaryDirectory(prefix='obk-alexa-')
build = Path(TEMP.name)
# Replace only platform include directives; compile the complete driver bodies
# separately, using the real HTTP request ABI and production jsmn parser.
for name in ('hue', 'wemo'):
    code = (ROOT / f'src/driver/drv_{name}.c').read_text()
    code = re.sub(r'^#include .*$', '', code, flags=re.M)
    (build / f'{name}.c').write_text('#include "host.h"\n' + code)
# Exercise the exact SSDP dispatch block with its UDP receive replaced by input.
ssdp = (ROOT/'src/driver/drv_ssdp.c').read_text()
start = ssdp.index('    if (!strncmp(udp_msgbuf, "M-SEARCH", 8)){')
end = ssdp.index('    // our NOTIFTY', start)
(build/'dispatch.c').write_text('#include "host.h"\nvoid search(const char *udp_msgbuf) {\n'
                               'struct sockaddr_in addr = {0};\n' + ssdp[start:end] + '}\n')
subprocess.run(['gcc', '-D_GNU_SOURCE', '-DENABLE_DRIVER_HUE=1', '-DENABLE_DRIVER_WEMO=1', '-DENABLE_LED_BASIC=1', '-shared', '-fPIC', '-std=gnu99', '-Wall', '-Wextra', '-Werror',
                '-I'+str(ROOT), '-I'+str(ROOT/'tests/alexa'), str(build/'hue.c'),
                str(build/'wemo.c'), str(build/'dispatch.c'), str(ROOT/'tests/alexa/host.c'),
                str(ROOT/'src/jsmn/jsmn.c'), '-o', str(build/'alexa.so')], check=True)
lib = c.CDLL(str(build/'alexa.so'))
lib.call.argtypes = [c.c_int, c.c_int, c.c_char_p, c.c_char_p, c.c_int, c.c_int]
lib.call.restype = c.c_char_p
lib.packet.argtypes = [c.c_int]
lib.packet.restype = c.c_char_p
lib.init()
# Read method values from the production HTTP header.
header = (ROOT/'src/httpserver/new_http.h').read_text()
methods = {name: int(value) for name, value in re.findall(r'HTTP_(GET|POST|PUT)\s*=\s*(\d+)', header)}

class Alexa(unittest.TestCase):
    def setUp(self):
        lib.reset()
        self.values = (c.c_int * 64).in_dll(lib, 'values')
        self.outputs = (c.c_int * 64).in_dll(lib, 'outputs')

    def call(self, url, method='GET', body=None, wemo=0, content=None):
        if isinstance(body, str): body = body.encode()
        return lib.call(wemo, methods[method], url.encode(), body, len(body or b''),
                        len(body or b'') if content is None else content).decode()

    def lights(self):
        return json.loads(self.call('api/FFFFFF/lights'))

    def test_auth_and_routes(self):
        self.assertEqual('FFFFFF', json.loads(self.call('/api', 'POST', '{}'))[0]['success']['username'])
        self.assertIn('username', json.loads(self.call('api/', 'POST', '{}'))[0]['success'])
        self.assertEqual('', self.call('apiary'))
        for url in ('api', 'api/FFFFFF/missing', 'api/FFFFFF/lights/nope', 'api/FFFFFF/lights/99999999999999'):
            self.assertIn('error', json.loads(self.call(url))[0])

    def test_native_api_routes_remain_available(self):
        for url in ('api/info', 'api/pins', 'api/channels', 'api/channelTypes',
                    'api/lfs/autoexec.bat', 'api/flash/0-1000', 'api/ota',
                    'api/reboot', 'api/cmnd', 'api/lfs/lights', 'api/run/lights',
                    'api/del/config', 'api/flash/config'):
            self.assertEqual('', self.call(url))
            self.assertEqual('', self.call(url, 'POST', '{}'))

    def test_discovery_configuration_without_registration(self):
        expected = json.loads(self.call('api/FFFFFF/config'))
        for route in ('api/config', 'api/config/', 'api/nouser/config',
                      'api/cached-alexa-user/config'):
            self.assertEqual(expected, json.loads(self.call(route)))
        for user in ('nouser', 'cached-alexa-user', 'ffffff'):
            self.assertEqual(self.lights(), json.loads(self.call(f'api/{user}/lights')))
            i = next(iter(self.lights()))
            response = json.loads(self.call(f'api/{user}/lights/{i}/state', 'PUT', '{"on":true}'))
            self.assertIn('success', response[0])
        self.assertEqual(self.lights(), json.loads(self.call('api/nouser'))['lights'])
        self.assertEqual('', self.call('api/new-native-endpoint'))
        self.assertEqual('', self.call('api/new-native-endpoint/file'))

    def test_list_and_identity(self):
        lights = self.lights()
        self.assertEqual(3, len(lights))
        self.assertEqual(3, len({v['uniqueid'] for v in lights.values()}))
        for value in lights.values():
            self.assertRegex(value['uniqueid'], r'^(?:[0-9a-fA-F]{2}:){7}[0-9a-fA-F]{2}-01$')
        self.assertTrue(all(0 < int(i) <= 2147483647 for i in lights))
        self.assertEqual('Lampe "salon"', next(iter(lights.values()))['name'])
        self.assertTrue(all(v['type']=='On/Off light' for v in lights.values()))

    def test_all_channels_have_distinct_ids(self):
        for ch in range(64): self.outputs[ch] = 1
        self.assertEqual(64, len(self.lights()))
        self.assertEqual(64, len({v["uniqueid"] for v in self.lights().values()}))

    def test_control_and_get_state(self):
        ids = list(self.lights())
        for i, channel in zip(ids, (1, 2, 63)):
            response = json.loads(self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', '{ "on" : true }'))
            self.assertTrue(response[0]['success'][f'/lights/{i}/state/on'])
            self.assertEqual(1, self.values[channel])
            self.assertEqual(1, json.loads(self.call(f'api/FFFFFF/lights/{i}'))['state']['on'])
            self.assertTrue(json.loads(self.call(f'api/FFFFFF/lights/{i}/state'))['on'])
            self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', '{"on":false}')
            self.assertEqual(0, self.values[channel])
        self.assertEqual([0]*64, list(self.values))

    def test_invalid_body_does_not_switch(self):
        i = next(iter(self.lights()))
        for body in (None, '', '{"on":', '{"on":"true"}', '{"on":1}',
                     '{"on":true,"bri":254}', '{"on":true,"on":false}',
                     '{"on":{"on":true}}', '[]', '{}', '{"on":true} garbage',
                     '{"on":true,}', '{"on" true}', '{"on":true false}'):
            with self.subTest(body=body):
                response = json.loads(self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', body))
                self.assertIn('error', response[0])
                self.assertEqual(0, self.values[1])
        self.assertIn('error', json.loads(self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', '{"on":true}', content=100))[0])

    def test_transition_metadata(self):
        i = next(iter(self.lights()))
        for body in ('{"on":true,"transitiontime":0}', '{"transitiontime":4,"on":true}'):
            reply = json.loads(self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', body))
            self.assertEqual(2, len(reply))
            self.assertEqual(1, self.values[1])
        for value in ('-1', '65536', '1.2', 'true', '"0"', '00'):
            self.values[1] = 0
            reply = json.loads(self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', '{"on":true,"transitiontime":'+value+'}'))
            self.assertIn('error', reply[0])
            self.assertEqual(0, self.values[1])

    def test_state_validation_is_atomic(self):
        i = next(iter(self.lights()))
        for body in ('{"on":true,"transitiontime":0} trailing',
                     '{"on":true,"transitiontime":0,}',
                     '{"on":true "transitiontime":0}',
                     '{"transitiontime":0,"on":true,}',
                     '{"on":true,"transitiontime":0,"transitiontime":1}'):
            with self.subTest(body=body):
                reply = json.loads(self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', body))
                self.assertIn('error', reply[0])
                self.assertTrue(reply[0]['error']['description'])
                self.assertEqual([0]*64, list(self.values))

    def test_fragmented_body(self):
        i = next(iter(self.lights()))
        for prefix in (b'', b'{"on":'):
            reader, writer = socket.socketpair()
            fd = c.c_int.in_dll(lib, 'test_fd')
            fd.value = reader.fileno()
            body = b'{"on":true,"transitiontime":0}'
            def send_rest():
                time.sleep(0.05)
                writer.sendall(body[len(prefix):])
            thread = threading.Thread(target=send_rest)
            thread.start()
            try:
                reply = json.loads(self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', prefix, content=len(body)))
                self.assertIn('success', reply[0])
                self.assertEqual(1, self.values[1])
            finally:
                thread.join()
                fd.value = 0
                reader.close()
                writer.close()
        self.assertIn('error', json.loads(self.call(f'api/FFFFFF/lights/{i}/state', 'PUT', '{}', content=513))[0])

    def test_wrong_device_and_input_channels(self):
        i = int(next(iter(self.lights())))
        for bad in (i ^ 128, (i & ~127) | 1, 0, 2147483648):
            response = json.loads(self.call(f'api/FFFFFF/lights/{bad}/state', 'PUT', '{"on":true}'))
            self.assertIn('error', response[0])
        self.assertEqual([0]*64, list(self.values))

    def test_global_and_config(self):
        all_data = json.loads(self.call('api/FFFFFF'))
        self.assertEqual(self.lights(), all_data['lights'])
        self.assertEqual('192.0.2.10', all_data['config']['ipaddress'])
        self.assertEqual('501234FFFEFFFFFF', all_data['config']['bridgeid'])
        self.assertEqual('50:12:34:ff:ff:ff', all_data['config']['mac'])
        self.assertEqual(all_data['config'], json.loads(self.call('api/FFFFFF/config')))
        for name in ('groups', 'schedules', 'scenes', 'sensors', 'rules', 'resourcelinks'):
            self.assertEqual({}, json.loads(self.call('api/FFFFFF/'+name)))

    def test_config_fits_http_format_buffer(self):
        ip = (c.c_char * 16).in_dll(lib, 'test_ip')
        ip.value = b'255.255.255.255'
        config = json.loads(self.call('api/config'))
        self.assertEqual('255.255.255.255', config['ipaddress'])
        self.assertEqual({'FFFFFF': {'name': 'Alexa'}}, config['whitelist'])
        self.assertEqual(config, json.loads(self.call('api/FFFFFF'))['config'])

    def test_stopped_driver_and_reinit(self):
        c.c_int.in_dll(lib, 'running').value = 0
        self.assertEqual('', self.call('api/FFFFFF/lights'))
        lib.init()
        c.c_int.in_dll(lib, 'running').value = 1
        self.assertEqual(3, len(self.lights()))

    def test_hue_discovery_datagrams(self):
        lib.DRV_HUE_Send_Advert_To(None)
        uuid = 'f6543a06-da50-11ba-8d8f-501234FFFFFF'
        targets = ('upnp:rootdevice', 'uuid:'+uuid, 'urn:schemas-upnp-org:device:Basic:1')
        for i in range(3):
            data = lib.packet(i).decode()
            self.assertTrue(data.startswith('HTTP/1.1 200 OK\r\n'))
            self.assertIn('LOCATION: http://192.0.2.10:80/description.xml\r\n', data)
            self.assertIn('hue-bridgeid: 501234FFFEFFFFFF\r\n', data)
            self.assertIn('\r\nST: ', data)
            self.assertIn('\r\nUSN: uuid:', data)
            self.assertTrue(data.endswith('\r\n\r\n'))
            self.assertIn(f'\r\nST: {targets[i]}\r\n', data)
            suffix = '' if i == 1 else '::'+targets[i]
            self.assertIn(f'\r\nUSN: uuid:{uuid}{suffix}\r\n', data)
            if i == 2:
                self.assertIn('::urn:schemas-upnp-org:device:Basic:1\r\n', data)

    def test_both_emulators_answer_generic_search(self):
        for target in ('ssdp:all', 'upnp:rootdevice'):
            lib.reset()
            lib.search(f'M-SEARCH * HTTP/1.1\r\nST: {target}\r\n\r\n'.encode())
            packets = [lib.packet(i).decode() for i in range(4)]
            self.assertIn('/setup.xml', packets[0])
            self.assertTrue(all('/description.xml' in p for p in packets[1:]))
            self.assertIsNone(lib.packet(4))

    def test_specific_search_and_fallback(self):
        lib.search(b'M-SEARCH * HTTP/1.1\r\nST: urn:Belkin:device:controllee:1\r\n\r\n')
        self.assertIn(b'ST: urn:Belkin:device:controllee:1\r\n', lib.packet(0))
        self.assertIsNone(lib.packet(1))
        lib.reset()
        lib.search(b'M-SEARCH * HTTP/1.1\r\nST: urn:schemas-upnp-org:device:Basic:1\r\n\r\n')
        self.assertIn(b'/description.xml', lib.packet(0))
        self.assertIsNone(lib.packet(3))
        lib.reset()
        c.c_int.in_dll(lib, 'running').value = 0
        lib.search(b'M-SEARCH * HTTP/1.1\r\nST: ssdp:all\r\n\r\n')
        self.assertEqual(b'OpenBeken fallback', lib.packet(0))

    def test_wemo_descriptor(self):
        self.values[1] = 1
        root = ET.fromstring(self.call('setup.xml', wemo=2))
        def get(name): return root.find('.//{*}'+name).text
        self.assertEqual('20161234FFFFFF', get('serialNumber'))
        self.assertEqual('http://192.0.2.10:80/', get('presentationURL'))
        self.assertEqual('Switch & "Salon"', get('friendlyName'))
        self.assertEqual('1', get('binaryState'))

    def test_wemo_set_get_first_relay(self):
        body = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"><s:Body><SetBinaryState><BinaryState> 1 </BinaryState></SetBinaryState></s:Body></s:Envelope>'
        data = self.call('upnp/control/basicevent1', 'POST', body, wemo=1)
        self.assertIn('SetBinaryStateResponse', data)
        self.assertEqual(1, self.values[1])
        self.assertEqual(0, self.values[2])
        data = self.call('upnp/control/basicevent1', 'POST', '<GetBinaryState/>', wemo=1)
        self.assertIn('GetBinaryStateResponse', data)
        self.assertIn('<BinaryState>1</BinaryState>', data)

    def test_wemo_invalid(self):
        for body in (None, '', '<Unknown/>', '<SetBinaryState/>',
                     '<SetBinaryState><BinaryState>2</BinaryState></SetBinaryState>'):
            root = ET.fromstring(self.call('upnp/control/basicevent1', 'POST', body, wemo=1))
            self.assertIsNotNone(root.find('.//{*}Fault'))
            self.assertEqual([0]*64, list(self.values))

if __name__ == '__main__':
    unittest.main(verbosity=2)
