"""frame_tool.py testleri. Çalıştırma: python3 -m unittest discover tools"""
import unittest

from frame_tool import crc16, decode_frame, decode_tlv, describe, encode_frame, encode_tlv


class FrameToolTest(unittest.TestCase):
    def test_crc_check_value(self):
        self.assertEqual(crc16(b"123456789"), 0x29B1)

    def test_doc_tlv_example(self):
        # docs/03-protokol.md: seviye 1250 mm, vana açık
        self.assertEqual(encode_tlv({"level_mm": 1250, "valve": 1}).hex(" "), "01 02 04 e2 02 01 01")

    def test_roundtrip(self):
        frame = encode_frame(0x01, 0x1001, 42, encode_tlv({"level_mm": 900, "alarms": 1}))
        info = describe(decode_frame(frame))
        self.assertEqual(info["type"], "TELEMETRY")
        self.assertEqual(info["src"], "0x1001")
        self.assertEqual(info["telemetry"], {"level_mm": 900, "alarms": 1})

    def test_bad_crc(self):
        frame = bytearray(encode_frame(0x02, 0x1001, 1, b""))
        frame[-1] ^= 0xFF
        with self.assertRaises(ValueError):
            decode_frame(bytes(frame))

    def test_unknown_tag_kept(self):
        self.assertEqual(decode_tlv(bytes([0x40, 1, 7])), {"tag_0x40": 7})


if __name__ == "__main__":
    unittest.main()
