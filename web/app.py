#!/usr/bin/env python3
"""
科学计算器键盘模拟器 - Web版本
与Zephyr计算器通过FIFO通信
现代化Web GUI设计，模拟真实科学计算器外观
"""

from flask import Flask, render_template, request, jsonify
import os
import struct
from enum import IntEnum
import threading
import webbrowser
import time


class KeyCode(IntEnum):
    """与Zephyr应用程序匹配的键码"""
    KEY_NONE = 0
    
    # 数字键
    KEY0 = 1; KEY1 = 2; KEY2 = 3; KEY3 = 4; KEY4 = 5
    KEY5 = 6; KEY6 = 7; KEY7 = 8; KEY8 = 9; KEY9 = 10
    
    # 基本运算
    KEY_PLUS = 11        # +
    KEY_MINUS = 12       # -
    KEY_MULTIPLY = 13    # *
    KEY_DIVIDE = 14      # /
    KEY_EQUAL = 15       # =
    KEY_CLEAR = 16       # C/AC
    KEY_DOT = 17         # .
    KEY_BACKSPACE = 18   # Del
    
    # 科学函数
    KEY_SIN = 19; KEY_COS = 20; KEY_TAN = 21
    KEY_LOG = 22; KEY_LN = 23; KEY_SQRT = 24
    KEY_POWER = 25; KEY_FACTORIAL = 26; KEY_PI = 27
    KEY_E = 28; KEY_PAREN_LEFT = 29; KEY_PAREN_RIGHT = 30
    
    # Casio fx-991 特定按键
    KEY_SHIFT = 31; KEY_ALPHA = 32; KEY_MODE = 33
    KEY_ON_AC = 34; KEY_X_POW_Y = 35; KEY_X_POW_MINUS1 = 36
    KEY_LOG10 = 37; KEY_EXP = 38; KEY_PERCENT = 39
    KEY_ANS = 40; KEY_ENG = 41; KEY_SETUP = 42
    KEY_STAT = 43; KEY_MATRIX = 44; KEY_VECTOR = 45
    KEY_CMPLX = 46; KEY_BASE_N = 47; KEY_EQUATION = 48
    KEY_CALC = 49; KEY_SOLVE = 50; KEY_INTEGRATE = 51
    KEY_DIFF = 52; KEY_TABLE = 53; KEY_RESET = 54
    KEY_RAN_HASH = 55; KEY_DRG = 56; KEY_HYP = 57
    KEY_STO = 58; KEY_RCL = 59; KEY_CONST = 60
    KEY_CONV = 61; KEY_FUNC = 62; KEY_OPTN = 63


class FifoWriter:
    """FIFO通信类"""
    def __init__(self, fifo_path="/tmp/calculator_keypad_fifo"):
        self.fifo_path = fifo_path
        self._ensure_fifo_exists()
    
    def _ensure_fifo_exists(self):
        """确保FIFO文件存在"""
        if not os.path.exists(self.fifo_path):
            try:
                os.mkfifo(self.fifo_path)
                print(f"Created FIFO: {self.fifo_path}")
            except OSError as e:
                print(f"Failed to create FIFO: {e}")
    
    def send_key(self, key_code):
        """发送键码到FIFO"""
        try:
            with open(self.fifo_path, 'wb') as fifo:
                key_bytes = struct.pack('<I', int(key_code))
                fifo.write(key_bytes)
                fifo.flush()
                print(f"Sent key: {key_code.name} ({key_code.value})")
                return True
        except Exception as e:
            print(f"Failed to send key {key_code.name}: {e}")
            return False


# Flask应用初始化
app = Flask(__name__)
fifo_writer = FifoWriter()


@app.route('/')
def index():
    """主页面"""
    return render_template('casio_fx991.html')


@app.route('/send_key', methods=['POST'])
def send_key():
    """接收按键并发送到FIFO"""
    try:
        data = request.get_json()
        key_name = data.get('key')
        
        # 将字符串转换为KeyCode枚举
        try:
            key_code = KeyCode[key_name]
        except KeyError:
            return jsonify({'status': 'error', 'message': f'Invalid key: {key_name}'})
        
        # 发送到FIFO
        success = fifo_writer.send_key(key_code)
        
        return jsonify({
            'status': 'success' if success else 'error',
            'key': key_name,
            'value': key_code.value
        })
        
    except Exception as e:
        print(f"Error in send_key: {e}")
        return jsonify({'status': 'error', 'message': str(e)})


def open_browser():
    """延迟打开浏览器"""
    time.sleep(1.5)
    webbrowser.open('http://localhost:5000')


def main():
    """主函数"""
    print("Starting CASIO fx-991ES PLUS Calculator Simulator")
    print("=" * 55)
    print(f"FIFO: {fifo_writer.fifo_path}")
    print("Web Interface: http://localhost:5000")
    print("=" * 55)
    print("Features:")
    print("  • Full CASIO fx-991ES PLUS layout")
    print("  • SHIFT/ALPHA mode support")
    print("  • Keyboard shortcuts")
    print("  • Real-time communication with Zephyr")
    print("=" * 55)
    print("Press Ctrl+C to exit")
    print()
    
    # 启动浏览器（在后台线程中）
    browser_thread = threading.Thread(target=open_browser, daemon=True)
    browser_thread.start()
    
    try:
        # 启动Flask应用
        app.run(host='0.0.0.0', port=5000, debug=False)
    except KeyboardInterrupt:
        print("\nShutting down CASIO fx-991ES PLUS simulator...")


if __name__ == "__main__":
    main()
