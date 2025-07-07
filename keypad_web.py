#!/usr/bin/env python3
"""
科学计算器键盘模拟器 - Web版本
与Zephyr计算器通过FIFO通信
现代化Web GUI设计，模拟真实科学计算器外观
"""

from flask import Flask, render_template_string, request, jsonify
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


# Flask应用
app = Flask(__name__)
fifo_writer = FifoWriter()

# HTML模板 - Casio fx-991ES PLUS 样式
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CASIO fx-991ES PLUS Simulator</title>
    <style>
        body {
            font-family: 'Courier New', monospace;
            background: #f0f0f0;
            margin: 0;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
        }
        
        .calculator {
            background: #2a2a2a;
            border-radius: 15px;
            padding: 20px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            border: 3px solid #333;
            width: 300px;
        }
        
        .brand {
            text-align: center;
            color: #fff;
            font-weight: bold;
            font-size: 12px;
            margin-bottom: 5px;
        }
        
        .model {
            text-align: center;
            color: #ffa500;
            font-weight: bold;
            font-size: 10px;
            margin-bottom: 15px;
            letter-spacing: 1px;
        }
        
        .display {
            background: #8db388;
            border: 2px solid #333;
            border-radius: 5px;
            height: 60px;
            margin-bottom: 15px;
            position: relative;
            font-family: 'Courier New', monospace;
            color: #000;
            padding: 10px;
            display: flex;
            flex-direction: column;
            justify-content: center;
        }
        
        .display-text {
            font-size: 14px;
            text-align: right;
            font-weight: bold;
        }
        
        .keypad {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 4px;
        }
        
        .key {
            border: none;
            border-radius: 3px;
            font-family: 'Arial', sans-serif;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.1s;
            position: relative;
            padding: 8px 4px;
            min-height: 35px;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            text-align: center;
            font-size: 10px;
            line-height: 1.1;
        }
        
        .key:active {
            transform: scale(0.95);
            box-shadow: inset 0 2px 4px rgba(0,0,0,0.3);
        }
        
        /* 按键颜色分类 */
        .key-shift { background: #ff6b35; color: white; }
        .key-alpha { background: #4dabf7; color: white; }
        .key-function { background: #666; color: white; }
        .key-number { background: #555; color: white; }
        .key-operator { background: #888; color: white; }
        .key-special { background: #444; color: white; }
        .key-clear { background: #d63031; color: white; }
        
        /* 二级功能标识 */
        .shift-label {
            font-size: 7px;
            color: #ff6b35;
            position: absolute;
            top: 2px;
            left: 2px;
            font-weight: normal;
        }
        
        .alpha-label {
            font-size: 7px;
            color: #4dabf7;
            position: absolute;
            top: 2px;
            right: 2px;
            font-weight: normal;
        }
        
        .main-label {
            font-size: 10px;
            font-weight: bold;
            margin-top: 8px;
        }
        
        .key-large {
            grid-column: span 2;
        }
        
        .status-bar {
            background: #333;
            color: #fff;
            padding: 5px;
            margin-bottom: 10px;
            border-radius: 3px;
            font-size: 10px;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="calculator">
        <div class="brand">CASIO</div>
        <div class="model">fx-991ES PLUS</div>
        
        <div class="display">
            <div class="display-text" id="display">0</div>
        </div>
        
        <div class="status-bar">
            <span id="mode-indicator">COMP</span>
            <span style="float: right;" id="shift-alpha">・・・</span>
        </div>
        
        <div class="keypad">
            <!-- Row 1 -->
            <button class="key key-shift" onclick="sendKey('KEY_SHIFT')">
                <div class="main-label">SHIFT</div>
            </button>
            <button class="key key-alpha" onclick="sendKey('KEY_ALPHA')">
                <div class="main-label">ALPHA</div>
            </button>
            <button class="key key-special" onclick="sendKey('KEY_MODE')">
                <div class="shift-label">SETUP</div>
                <div class="main-label">MODE</div>
            </button>
            <button class="key key-clear" onclick="sendKey('KEY_ON_AC')">
                <div class="shift-label">OFF</div>
                <div class="main-label">ON</div>
            </button>
            <button class="key key-clear" onclick="sendKey('KEY_CLEAR')">
                <div class="main-label">AC</div>
            </button>
            
            <!-- Row 2 -->
            <button class="key key-function" onclick="sendKey('KEY_X_POW_Y')">
                <div class="shift-label">x√</div>
                <div class="main-label">x<sup>y</sup></div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_LOG')">
                <div class="shift-label">10<sup>x</sup></div>
                <div class="main-label">log</div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_LN')">
                <div class="shift-label">e<sup>x</sup></div>
                <div class="main-label">ln</div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_PAREN_LEFT')">
                <div class="shift-label">)</div>
                <div class="main-label">(</div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_PAREN_RIGHT')">
                <div class="shift-label">)</div>
                <div class="main-label">)</div>
            </button>
            
            <!-- Row 3 -->
            <button class="key key-function" onclick="sendKey('KEY_SQRT')">
                <div class="shift-label">x²</div>
                <div class="main-label">√</div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_SIN')">
                <div class="shift-label">sin⁻¹</div>
                <div class="main-label">sin</div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_COS')">
                <div class="shift-label">cos⁻¹</div>
                <div class="main-label">cos</div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_TAN')">
                <div class="shift-label">tan⁻¹</div>
                <div class="main-label">tan</div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_BACKSPACE')">
                <div class="shift-label">INS</div>
                <div class="main-label">DEL</div>
            </button>
            
            <!-- Row 4 -->
            <button class="key key-function" onclick="sendKey('KEY_X_POW_MINUS1')">
                <div class="shift-label">∛</div>
                <div class="main-label">x⁻¹</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY7')">
                <div class="alpha-label">G</div>
                <div class="main-label">7</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY8')">
                <div class="alpha-label">H</div>
                <div class="main-label">8</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY9')">
                <div class="alpha-label">I</div>
                <div class="main-label">9</div>
            </button>
            <button class="key key-operator" onclick="sendKey('KEY_DIVIDE')">
                <div class="shift-label">÷R</div>
                <div class="main-label">÷</div>
            </button>
            
            <!-- Row 5 -->
            <button class="key key-function" onclick="sendKey('KEY_PERCENT')">
                <div class="shift-label">%</div>
                <div class="main-label">%</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY4')">
                <div class="alpha-label">D</div>
                <div class="main-label">4</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY5')">
                <div class="alpha-label">E</div>
                <div class="main-label">5</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY6')">
                <div class="alpha-label">F</div>
                <div class="main-label">6</div>
            </button>
            <button class="key key-operator" onclick="sendKey('KEY_MULTIPLY')">
                <div class="shift-label">×</div>
                <div class="main-label">×</div>
            </button>
            
            <!-- Row 6 -->
            <button class="key key-function" onclick="sendKey('KEY_ENG')">
                <div class="shift-label">←</div>
                <div class="main-label">ENG</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY1')">
                <div class="alpha-label">A</div>
                <div class="main-label">1</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY2')">
                <div class="alpha-label">B</div>
                <div class="main-label">2</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY3')">
                <div class="alpha-label">C</div>
                <div class="main-label">3</div>
            </button>
            <button class="key key-operator" onclick="sendKey('KEY_MINUS')">
                <div class="shift-label">-</div>
                <div class="main-label">-</div>
            </button>
            
            <!-- Row 7 -->
            <button class="key key-function" onclick="sendKey('KEY_ANS')">
                <div class="shift-label">DRG</div>
                <div class="alpha-label">X</div>
                <div class="main-label">Ans</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY0')">
                <div class="alpha-label">SPACE</div>
                <div class="main-label">0</div>
            </button>
            <button class="key key-number" onclick="sendKey('KEY_DOT')">
                <div class="shift-label">,</div>
                <div class="alpha-label">"</div>
                <div class="main-label">.</div>
            </button>
            <button class="key key-function" onclick="sendKey('KEY_EXP')">
                <div class="shift-label">π</div>
                <div class="alpha-label">:</div>
                <div class="main-label">×10<sup>x</sup></div>
            </button>
            <button class="key key-operator" onclick="sendKey('KEY_PLUS')">
                <div class="shift-label">+</div>
                <div class="main-label">+</div>
            </button>
            
            <!-- Row 8 -->
            <button class="key key-operator key-large" onclick="sendKey('KEY_EQUAL')">
                <div class="main-label">=</div>
            </button>
            <button class="key key-special" onclick="sendKey('KEY_MATRIX')">
                <div class="main-label">MATRIX</div>
            </button>
            <button class="key key-special" onclick="sendKey('KEY_VECTOR')">
                <div class="main-label">VECTOR</div>
            </button>
            <button class="key key-special" onclick="sendKey('KEY_SOLVE')">
                <div class="main-label">SOLVE</div>
            </button>
        </div>
    </div>
    
    <script>
        let shiftMode = false;
        let alphaMode = false;
        
        function sendKey(keyName) {
            fetch('/send_key', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({key: keyName})
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    updateDisplay(keyName);
                    handleSpecialKeys(keyName);
                }
            })
            .catch(error => console.error('Error:', error));
        }
        
        function handleSpecialKeys(keyName) {
            const indicator = document.getElementById('shift-alpha');
            
            if (keyName === 'KEY_SHIFT') {
                shiftMode = !shiftMode;
                updateModeIndicator();
            } else if (keyName === 'KEY_ALPHA') {
                alphaMode = !alphaMode;
                updateModeIndicator();
            } else {
                // Other keys reset modes
                if (shiftMode || alphaMode) {
                    shiftMode = false;
                    alphaMode = false;
                    updateModeIndicator();
                }
            }
        }
        
        function updateModeIndicator() {
            const indicator = document.getElementById('shift-alpha');
            let text = '';
            
            if (shiftMode) text += 'S ';
            if (alphaMode) text += 'A ';
            if (!shiftMode && !alphaMode) text = '・・・';
            
            indicator.textContent = text;
        }
        
        function updateDisplay(keyName) {
            // This would be updated by the actual calculator logic
            console.log('Key pressed:', keyName);
        }
        
        // Keyboard support
        document.addEventListener('keydown', function(event) {
            const keyMap = {
                '0': 'KEY0', '1': 'KEY1', '2': 'KEY2', '3': 'KEY3', '4': 'KEY4',
                '5': 'KEY5', '6': 'KEY6', '7': 'KEY7', '8': 'KEY8', '9': 'KEY9',
                '+': 'KEY_PLUS', '-': 'KEY_MINUS', '*': 'KEY_MULTIPLY', '/': 'KEY_DIVIDE',
                '=': 'KEY_EQUAL', 'Enter': 'KEY_EQUAL', '.': 'KEY_DOT',
                'Backspace': 'KEY_BACKSPACE', 'Delete': 'KEY_CLEAR',
                'Escape': 'KEY_CLEAR'
            };
            
            if (keyMap[event.key]) {
                event.preventDefault();
                sendKey(keyMap[event.key]);
            }
        });
    </script>
</body>
</html>
"""


# Flask应用
app = Flask(__name__)
fifo_writer = FifoWriter()


@app.route('/')
def index():
    """主页面"""
    return render_template_string(HTML_TEMPLATE)


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
    print("Starting Scientific Calculator Keypad (Web Version)")
    print("=" * 50)
    print("FIFO: /tmp/calculator_keypad_fifo")
    print("Web Interface: http://localhost:5000")
    print("=" * 50)
    print("Use mouse clicks or keyboard shortcuts in the web browser")
    print("Press Ctrl+C to exit")
    print()
    
    # 启动浏览器（在后台线程中）
    browser_thread = threading.Thread(target=open_browser, daemon=True)
    browser_thread.start()
    
    try:
        # 启动Flask应用
        app.run(host='0.0.0.0', port=5000, debug=False)
    except KeyboardInterrupt:
        print("\nShutting down calculator keypad...")


if __name__ == "__main__":
    main()
