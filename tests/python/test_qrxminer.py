# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.
from unittest import TestCase
import time

from pyqrandomx import pyqrandomx
from pyqrandomx.pyqrandomx import QRXMiner
from pyqrandomx.pyqrandomx import ThreadedQRandomX
from pyqrandomx.pyqrandomx import PoWHelper


class TestQRXMiner(TestCase):
    def __init__(self, *args, **kwargs):
        super(TestQRXMiner, self).__init__(*args, **kwargs)

    def test_miner_simple(self):
        class CustomQMiner(QRXMiner):
            def __init__(self):
                QRXMiner.__init__(self)

            def handleEvent(self, event):
                if event.type == pyqrandomx.SOLUTION:
                    print("Hey a solution has been found!", event.nonce)
                    self.python_nonce = event.nonce
                return True
        qrx = ThreadedQRandomX()

        main_height = 10
        seed_height = qrx.getSeedHeight(main_height)

        seed_hash = [
            0x2a, 0x1c, 0x4a, 0x94, 0x33, 0xf1, 0xde, 0x36,
            0xf8, 0xb9, 0x9c, 0x7c, 0x5a, 0xce, 0xb7, 0xbd,
            0x2e, 0xb3, 0x9e, 0x1e, 0xad, 0x64, 0x8e, 0xa5,
            0x82, 0x27, 0xd3, 0x99, 0xad, 0x84, 0xc7, 0x24
        ]
        input_bytes = [
            0x00, 0x9d, 0x6b, 0x07, 0x38, 0x20, 0xb0, 0x0a,
            0x5b, 0x60, 0xc7, 0x64, 0x6f, 0x3d, 0x30, 0x46,
            0xa3, 0x49, 0x49, 0xcd, 0x93, 0x9b, 0x2b, 0x52,
            0xb9, 0x9f, 0x1b, 0xb2, 0xde, 0x1f, 0x4c, 0x92,
            0x12, 0x7a, 0xe9, 0xe4, 0xc3, 0xe3, 0x9e, 0xe0,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x58,
            0x22, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x54, 0xdb, 0x8c, 0x8e, 0xa6, 0x6f, 0xc0, 0x5b,
            0x1d, 0x12, 0x24, 0x25, 0x5a, 0x24, 0x70, 0x8e,
            0x5d, 0x67, 0x33, 0xdc
        ]
        target = [
            0x1E, 0xE5, 0x3F, 0xE1, 0xAC, 0xF3, 0x55, 0x92,
            0x66, 0xD8, 0x43, 0x89, 0xCE, 0xDE, 0x99, 0x33,
            0xC6, 0x8F, 0xC5, 0x1E, 0xD0, 0xA6, 0xC7, 0x91,
            0xF8, 0xF9, 0xE8, 0x9D, 0xB6, 0x23, 0xF0, 0x00
        ]

        # Create a customized miner
        qm = CustomQMiner()

        # Set input bytes, nonce
        qm.start(mainHeight=main_height,
                 seedHeight=seed_height,
                 seedHash=seed_hash,
                 input=input_bytes,
                 nonceOffset=0,
                 target=target,
                 thread_count=2)

        # Python can sleep or do something else.. the callback will happen in the background
        time.sleep(8)

        # This property has been just created in the python custom class when the event is received
        self.assertEqual(148, qm.python_nonce)

        # Now check wrapper values
        self.assertEqual(True, qm.solutionAvailable())
        self.assertEqual(148, qm.solutionNonce())

    def test_miner_verify(self):
        class CustomQMiner(QRXMiner):
            def __init__(self):
                QRXMiner.__init__(self)

            def handleEvent(self, event):
                if event.type == pyqrandomx.SOLUTION:
                    print("Hey a solution has been found!", event.nonce)
                    self.python_nonce = event.nonce
                return True

        qrx = ThreadedQRandomX()

        main_height = 10
        seed_height = qrx.getSeedHeight(main_height)

        seed_hash = [
            0x2a, 0x1c, 0x4a, 0x94, 0x33, 0xf1, 0xde, 0x36,
            0xf8, 0xb9, 0x9c, 0x7c, 0x5a, 0xce, 0xb7, 0xbd,
            0x2e, 0xb3, 0x9e, 0x1e, 0xad, 0x64, 0x8e, 0xa5,
            0x82, 0x27, 0xd3, 0x99, 0xad, 0x84, 0xc7, 0x24
        ]
        input_bytes = [
            0x00, 0x9d, 0x6b, 0x07, 0x38, 0x20, 0xb0, 0x0a,
            0x5b, 0x60, 0xc7, 0x64, 0x6f, 0x3d, 0x30, 0x46,
            0xa3, 0x49, 0x49, 0xcd, 0x93, 0x9b, 0x2b, 0x52,
            0xb9, 0x9f, 0x1b, 0xb2, 0xde, 0x1f, 0x4c, 0x92,
            0x12, 0x7a, 0xe9, 0xe4, 0xc3, 0xe3, 0x9e, 0xe0,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x58,
            0x22, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x54, 0xdb, 0x8c, 0x8e, 0xa6, 0x6f, 0xc0, 0x5b,
            0x1d, 0x12, 0x24, 0x25, 0x5a, 0x24, 0x70, 0x8e,
            0x5d, 0x67, 0x33, 0xdc
        ]
        target = [
            0x1E, 0xE5, 0x3F, 0xE1, 0xAC, 0xF3, 0x55, 0x92,
            0x66, 0xD8, 0x43, 0x89, 0xCE, 0xDE, 0x99, 0x33,
            0xC6, 0x8F, 0xC5, 0x1E, 0xD0, 0xA6, 0xC7, 0x91,
            0xF8, 0xF9, 0xE8, 0x9D, 0xB6, 0x23, 0xF0, 0x0C
        ]

        # Create a customized miner
        qm = CustomQMiner()

        # Set input bytes, nonce
        qm.start(mainHeight=main_height,
                 seedHeight=seed_height,
                 seedHash=seed_hash,
                 input=input_bytes,
                 nonceOffset=0,
                 target=target,
                 thread_count=2)

        # verifying at different seed hash while mining is going on for different seed hash
        ph = PoWHelper()
        block_number2 = 811073
        seed_block_number2 = 811008
        seed_header_hash2 = bytes.fromhex('f7e2ebf0f08f01e8d08439303512456d2fd6bc276395c97f22e75b22efcd0f00')
        mining_blob2 = bytes.fromhex('00ba9cf950493934bdde9b954f7aa3e28581c45b00bc657fc26f445e4b542f191849948a9ab444000000ab000000000000000000000000000f5a315073a01c1aed2e0d5a61d148e742f13023')
        target2 = bytes.fromhex('1100efddccbbaa99887766554433221100efddccbbaa99887766554433221100')

        self.assertTrue(ph.verifyInput(block_number2, seed_block_number2, seed_header_hash2, mining_blob2, target2))

        # Python can sleep or do something else.. the callback will happen in the background
        time.sleep(8)

        # This property has been just created in the python custom class when the event is received
        # Solution may vary depending upon which thread has first reached to the solution
        self.assertEqual(True, qm.solutionAvailable())

        solution_input = list(qm.solutionInput())

        print("input_bytes    ", input_bytes)
        print("solution_input ", solution_input)
        print("target         ", target)
        print("solutionHash   ", qm.solutionHash())

        output = qrx.hash(main_height, seed_height, seed_hash, solution_input, 0)
        print("raw     Hash   ", output)

        self.assertTrue(ph.verifyInput(main_height, seed_height, seed_hash, solution_input, target))
        self.assertIn(qm.python_nonce, [72, 73])
        self.assertIn(qm.solutionNonce(), [72, 73])
        solution_input[4] = 0x29
        self.assertFalse(ph.verifyInput(main_height, seed_height, seed_hash, solution_input, target))

    def test_miner_timeout(self):
        class CustomQMiner(QRXMiner):
            def __init__(self):
                QRXMiner.__init__(self)
                self.timeout_triggered = False

            def handleEvent(self, event):
                if event.type == pyqrandomx.TIMEOUT:
                    print("Timeout")
                    self.timeout_triggered = True
                return True

        qrx = ThreadedQRandomX()

        main_height = 10
        seed_height = qrx.getSeedHeight(main_height)

        seed_hash = [
            0x2a, 0x1c, 0x4a, 0x94, 0x33, 0xf1, 0xde, 0x36,
            0xf8, 0xb9, 0x9c, 0x7c, 0x5a, 0xce, 0xb7, 0xbd,
            0x2e, 0xb3, 0x9e, 0x1e, 0xad, 0x64, 0x8e, 0xa5,
            0x82, 0x27, 0xd3, 0x99, 0xad, 0x84, 0xc7, 0x24
        ]
        input_bytes = [
            0x00, 0x9d, 0x6b, 0x07, 0x38, 0x20, 0xb0, 0x0a,
            0x5b, 0x60, 0xc7, 0x64, 0x6f, 0x3d, 0x30, 0x46,
            0xa3, 0x49, 0x49, 0xcd, 0x93, 0x9b, 0x2b, 0x52,
            0xb9, 0x9f, 0x1b, 0xb2, 0xde, 0x1f, 0x4c, 0x92,
            0x12, 0x7a, 0xe9, 0xe4, 0xc3, 0xe3, 0x9e, 0xe0,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x58,
            0x22, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x54, 0xdb, 0x8c, 0x8e, 0xa6, 0x6f, 0xc0, 0x5b,
            0x1d, 0x12, 0x24, 0x25, 0x5a, 0x24, 0x70, 0x8e,
            0x5d, 0x67, 0x33, 0xdc
        ]
        target = [
            0x1E, 0xE5, 0x3F, 0xE1, 0xAC, 0xF3, 0x55, 0x92,
            0x66, 0xD8, 0x43, 0x89, 0xCE, 0xDE, 0x99, 0x33,
            0xC6, 0x8F, 0xC5, 0x1E, 0xD0, 0xA6, 0xC7, 0x91,
            0xF8, 0xF9, 0xE8, 0x9D, 0x00, 0x00, 0x00, 0x00
        ]

        # Create a customized miner
        qm = CustomQMiner()

        # Set input bytes, nonce
        qm.start(mainHeight=main_height,
                 seedHeight=seed_height,
                 seedHash=seed_hash,
                 input=input_bytes,
                 nonceOffset=0,
                 target=target,
                 thread_count=2)

        qm.setTimer(2000)

        # Python can sleep or do something else.. the callback will happen in the background
        time.sleep(5)

        # This property has been just created in the python custom class when the event is received
        self.assertTrue(qm.timeout_triggered)

    def test_miner_timeout_extend(self):
        class CustomQMiner(QRXMiner):
            def __init__(self):
                QRXMiner.__init__(self)
                self.timeout_triggered = False

            def handleEvent(self, event):
                if event.type == pyqrandomx.TIMEOUT:
                    print("Timeout")
                    self.timeout_triggered = True
                return True

        qrx = ThreadedQRandomX()

        main_height = 10
        seed_height = qrx.getSeedHeight(main_height)

        seed_hash = [
            0x2a, 0x1c, 0x4a, 0x94, 0x33, 0xf1, 0xde, 0x36,
            0xf8, 0xb9, 0x9c, 0x7c, 0x5a, 0xce, 0xb7, 0xbd,
            0x2e, 0xb3, 0x9e, 0x1e, 0xad, 0x64, 0x8e, 0xa5,
            0x82, 0x27, 0xd3, 0x99, 0xad, 0x84, 0xc7, 0x24
        ]
        input_bytes = [
            0x00, 0x9d, 0x6b, 0x07, 0x38, 0x20, 0xb0, 0x0a,
            0x5b, 0x60, 0xc7, 0x64, 0x6f, 0x3d, 0x30, 0x46,
            0xa3, 0x49, 0x49, 0xcd, 0x93, 0x9b, 0x2b, 0x52,
            0xb9, 0x9f, 0x1b, 0xb2, 0xde, 0x1f, 0x4c, 0x92,
            0x12, 0x7a, 0xe9, 0xe4, 0xc3, 0xe3, 0x9e, 0xe0,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x58,
            0x22, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x54, 0xdb, 0x8c, 0x8e, 0xa6, 0x6f, 0xc0, 0x5b,
            0x1d, 0x12, 0x24, 0x25, 0x5a, 0x24, 0x70, 0x8e,
            0x5d, 0x67, 0x33, 0xdc
        ]
        target = [
            0x1E, 0xE5, 0x3F, 0xE1, 0xAC, 0xF3, 0x55, 0x92,
            0x66, 0xD8, 0x43, 0x89, 0xCE, 0xDE, 0x99, 0x33,
            0xC6, 0x8F, 0xC5, 0x1E, 0xD0, 0xA6, 0xC7, 0x91,
            0xF8, 0xF9, 0xE8, 0x9D, 0x00, 0x00, 0x00, 0x00
        ]

        # Create a customized miner
        qm = CustomQMiner()

        # Set input bytes, nonce
        qm.start(mainHeight=main_height,
                 seedHeight=seed_height,
                 seedHash=seed_hash,
                 input=input_bytes,
                 nonceOffset=0,
                 target=target,
                 thread_count=2)

        qm.setTimer(2000)
        time.sleep(1)
        qm.setTimer(4000)
        time.sleep(3)

        # This property has been just created in the python custom class when the event is received
        self.assertFalse(qm.timeout_triggered)

    def test_miner_timeout_disable(self):
        class CustomQMiner(QRXMiner):
            def __init__(self):
                QRXMiner.__init__(self)
                self.timeout_triggered = False

            def handleEvent(self, event):
                if event.type == pyqrandomx.TIMEOUT:
                    print("Timeout")
                    self.timeout_triggered = True
                return True

        qrx = ThreadedQRandomX()

        main_height = 10
        seed_height = qrx.getSeedHeight(main_height)

        seed_hash = [
            0x2a, 0x1c, 0x4a, 0x94, 0x33, 0xf1, 0xde, 0x36,
            0xf8, 0xb9, 0x9c, 0x7c, 0x5a, 0xce, 0xb7, 0xbd,
            0x2e, 0xb3, 0x9e, 0x1e, 0xad, 0x64, 0x8e, 0xa5,
            0x82, 0x27, 0xd3, 0x99, 0xad, 0x84, 0xc7, 0x24
        ]
        input_bytes = [
            0x00, 0x9d, 0x6b, 0x07, 0x38, 0x20, 0xb0, 0x0a,
            0x5b, 0x60, 0xc7, 0x64, 0x6f, 0x3d, 0x30, 0x46,
            0xa3, 0x49, 0x49, 0xcd, 0x93, 0x9b, 0x2b, 0x52,
            0xb9, 0x9f, 0x1b, 0xb2, 0xde, 0x1f, 0x4c, 0x92,
            0x12, 0x7a, 0xe9, 0xe4, 0xc3, 0xe3, 0x9e, 0xe0,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x58,
            0x22, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x54, 0xdb, 0x8c, 0x8e, 0xa6, 0x6f, 0xc0, 0x5b,
            0x1d, 0x12, 0x24, 0x25, 0x5a, 0x24, 0x70, 0x8e,
            0x5d, 0x67, 0x33, 0xdc
        ]
        target = [
            0x1E, 0xE5, 0x3F, 0xE1, 0xAC, 0xF3, 0x55, 0x92,
            0x66, 0xD8, 0x43, 0x89, 0xCE, 0xDE, 0x99, 0x33,
            0xC6, 0x8F, 0xC5, 0x1E, 0xD0, 0xA6, 0xC7, 0x91,
            0xF8, 0xF9, 0xE8, 0x9D, 0x00, 0x00, 0x00, 0x00
        ]

        # Create a customized miner
        qm = CustomQMiner()

        # Set input bytes, nonce
        qm.start(mainHeight=main_height,
                 seedHeight=seed_height,
                 seedHash=seed_hash,
                 input=input_bytes,
                 nonceOffset=0,
                 target=target,
                 thread_count=2)

        qm.setTimer(500)
        qm.disableTimer()
        time.sleep(1)

        # This property has been just created in the python custom class when the event is received
        self.assertFalse(qm.timeout_triggered)
