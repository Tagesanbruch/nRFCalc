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
    NONE = 0
    
    # 数字键
    KEY0 = 1; KEY1 = 2; KEY2 = 3; KEY3 = 4; KEY4 = 5
    KEY5 = 6; KEY6 = 7; KEY7 = 8; KEY8 = 9; KEY9 = 10
    
    # 基本运算
    KEY_PLUS = 11        # +
    KEY_MINUS = 12       # -
    KEY_MULTIPLY = 13    # *
    KEY_DIVIDE = 14      # /
    KEY_EQUAL = 15       # =
    KEY_CLEAR = 16       # C
    KEY_DOT = 17         # .
    KEY_BACKSPACE = 18   # Del
    
    # 科学函数
    KEY_SIN = 19
    KEY_COS = 20
    KEY_TAN = 21
    KEY_LOG = 22
    KEY_LN = 23
    KEY_SQRT = 24
    KEY_POWER = 25
    KEY_FACTORIAL = 26
    KEY_PI = 27
    KEY_E = 28
    KEY_PAREN_LEFT = 29
    KEY_PAREN_RIGHT = 30


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

# HTML模板
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Scientific Calculator Keypad</title>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            background: linear-gradient(135deg, #1a1a1a, #2d2d2d);
            margin: 0;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            color: white;
        }
        
        .calculator-container {
            background: #2d2d2d;
            border-radius: 15px;
            padding: 30px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            max-width: 500px;
            width: 100%;
        }
        
        .header {
            text-align: center;
            margin-bottom: 20px;
        }
        
        .title {
            color: #ff8c42;
            font-size: 24px;
            font-weight: bold;
            margin-bottom: 10px;
        }
        
        .status {
            color: #66ff66;
            font-size: 14px;
            margin-bottom: 20px;
        }
        
        .keypad {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 10px;
            margin-top: 20px;
        }
        
        .btn {
            border: none;
            border-radius: 8px;
            padding: 15px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.2s;
            outline: none;
            user-select: none;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.3);
        }
        
        .btn:active {
            transform: translateY(0);
            box-shadow: 0 2px 5px rgba(0,0,0,0.3);
        }
        
        .btn-number {
            background: #666666;
            color: white;
            font-size: 18px;
        }
        
        .btn-number:hover {
            background: #777777;
        }
        
        .btn-operation {
            background: #ff8c42;
            color: white;
            font-size: 18px;
        }
        
        .btn-operation:hover {
            background: #ff9c52;
        }
        
        .btn-function {
            background: #4a4a4a;
            color: #ff8c42;
            font-size: 14px;
        }
        
        .btn-function:hover {
            background: #5a5a5a;
        }
        
        .btn-scientific {
            background: #5a5a5a;
            color: #66ff66;
            font-size: 14px;
        }
        
        .btn-scientific:hover {
            background: #6a6a6a;
        }
        
        .btn-clear {
            background: #e74c3c;
            color: white;
            font-size: 16px;
        }
        
        .btn-clear:hover {
            background: #f75c4c;
        }
        
        .btn-equal {
            background: #27ae60;
            color: white;
            font-size: 20px;
            grid-row: span 4;
        }
        
        .btn-equal:hover {
            background: #37be70;
        }
        
        .btn-wide {
            grid-column: span 2;
        }
        
        .feedback {
            position: fixed;
            top: 20px;
            right: 20px;
            background: #333;
            color: #66ff66;
            padding: 10px 20px;
            border-radius: 5px;
            display: none;
            z-index: 1000;
        }
        
        .feedback.error {
            color: #e74c3c;
        }
    </style>
</head>
<body>
    <div class="calculator-container">
        <div class="header">
            <div class="title">SCIENTIFIC CALCULATOR</div>
            <div class="status" id="status">Ready - Waiting for Zephyr Calculator</div>
        </div>
        
        <div class="keypad">
            <!-- 第1行：科学函数 -->
            <button class="btn btn-function" data-key="KEY_SIN" data-text="sin">sin</button>
            <button class="btn btn-function" data-key="KEY_COS" data-text="cos">cos</button>
            <button class="btn btn-function" data-key="KEY_TAN" data-text="tan">tan</button>
            <button class="btn btn-function" data-key="KEY_LOG" data-text="log">log</button>
            <button class="btn btn-function" data-key="KEY_LN" data-text="ln">ln</button>
            
            <!-- 第2行：更多科学函数 -->
            <button class="btn btn-scientific" data-key="KEY_SQRT" data-text="√">√</button>
            <button class="btn btn-scientific" data-key="KEY_POWER" data-text="x²">x²</button>
            <button class="btn btn-scientific" data-key="KEY_FACTORIAL" data-text="x!">x!</button>
            <button class="btn btn-scientific" data-key="KEY_PI" data-text="π">π</button>
            <button class="btn btn-scientific" data-key="KEY_E" data-text="e">e</button>
            
            <!-- 第3行：括号和清除 -->
            <button class="btn btn-scientific" data-key="KEY_PAREN_LEFT" data-text="(">(</button>
            <button class="btn btn-scientific" data-key="KEY_PAREN_RIGHT" data-text=")">)</button>
            <button class="btn btn-clear btn-wide" data-key="KEY_CLEAR" data-text="AC">AC</button>
            <button class="btn btn-operation" data-key="KEY_BACKSPACE" data-text="⌫">⌫</button>
            
            <!-- 第4行：数字和运算 -->
            <button class="btn btn-number" data-key="KEY7" data-text="7">7</button>
            <button class="btn btn-number" data-key="KEY8" data-text="8">8</button>
            <button class="btn btn-number" data-key="KEY9" data-text="9">9</button>
            <button class="btn btn-operation" data-key="KEY_DIVIDE" data-text="÷">÷</button>
            
            <!-- 等号按钮（跨4行） -->
            <button class="btn btn-equal" data-key="KEY_EQUAL" data-text="=">=</button>
            
            <!-- 第5行 -->
            <button class="btn btn-number" data-key="KEY4" data-text="4">4</button>
            <button class="btn btn-number" data-key="KEY5" data-text="5">5</button>
            <button class="btn btn-number" data-key="KEY6" data-text="6">6</button>
            <button class="btn btn-operation" data-key="KEY_MULTIPLY" data-text="×">×</button>
            
            <!-- 第6行 -->
            <button class="btn btn-number" data-key="KEY1" data-text="1">1</button>
            <button class="btn btn-number" data-key="KEY2" data-text="2">2</button>
            <button class="btn btn-number" data-key="KEY3" data-text="3">3</button>
            <button class="btn btn-operation" data-key="KEY_MINUS" data-text="-">-</button>
            
            <!-- 第7行 -->
            <button class="btn btn-number btn-wide" data-key="KEY0" data-text="0">0</button>
            <button class="btn btn-number" data-key="KEY_DOT" data-text=".">.</button>
            <button class="btn btn-operation" data-key="KEY_PLUS" data-text="+">+</button>
        </div>
    </div>
    
    <div class="feedback" id="feedback"></div>
    
    <script>
        // 键盘事件映射
        const keyMap = {
            '0': 'KEY0', '1': 'KEY1', '2': 'KEY2', '3': 'KEY3', '4': 'KEY4',
            '5': 'KEY5', '6': 'KEY6', '7': 'KEY7', '8': 'KEY8', '9': 'KEY9',
            '+': 'KEY_PLUS', '-': 'KEY_MINUS', '*': 'KEY_MULTIPLY', '/': 'KEY_DIVIDE',
            '=': 'KEY_EQUAL', '.': 'KEY_DOT', 'c': 'KEY_CLEAR', 'C': 'KEY_CLEAR'
        };
        
        const specialKeys = {
            'Enter': 'KEY_EQUAL',
            'Backspace': 'KEY_BACKSPACE',
            'Delete': 'KEY_CLEAR'
        };
        
        // 发送按键到服务器
        async function sendKey(keyCode, keyText) {
            try {
                const response = await fetch('/send_key', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        key_code: keyCode,
                        key_text: keyText
                    })
                });
                
                const result = await response.json();
                
                if (result.success) {
                    document.getElementById('status').textContent = 
                        `Sent: ${keyText} → Zephyr Calculator`;
                    document.getElementById('status').style.color = '#66ff66';
                    showFeedback(`Sent: ${keyText}`, false);
                } else {
                    document.getElementById('status').textContent = 
                        'Error: Failed to send to Zephyr Calculator';
                    document.getElementById('status').style.color = '#e74c3c';
                    showFeedback('Error: Failed to send key', true);
                }
                
                // 3秒后恢复默认状态
                setTimeout(() => {
                    document.getElementById('status').textContent = 
                        'Ready - Waiting for Zephyr Calculator';
                    document.getElementById('status').style.color = '#66ff66';
                }, 3000);
                
            } catch (error) {
                console.error('Error sending key:', error);
                showFeedback('Network error', true);
            }
        }
        
        // 显示反馈消息
        function showFeedback(message, isError = false) {
            const feedback = document.getElementById('feedback');
            feedback.textContent = message;
            feedback.className = isError ? 'feedback error' : 'feedback';
            feedback.style.display = 'block';
            
            setTimeout(() => {
                feedback.style.display = 'none';
            }, 2000);
        }
        
        // 按钮点击事件
        document.querySelectorAll('.btn').forEach(button => {
            button.addEventListener('click', () => {
                const keyCode = button.dataset.key;
                const keyText = button.dataset.text;
                sendKey(keyCode, keyText);
            });
        });
        
        // 键盘事件
        document.addEventListener('keydown', (event) => {
            let keyCode = null;
            let keyText = event.key;
            
            if (event.key in keyMap) {
                keyCode = keyMap[event.key];
            } else if (event.key in specialKeys) {
                keyCode = specialKeys[event.key];
                keyText = event.key === 'Enter' ? '=' : 
                         event.key === 'Backspace' ? '⌫' : 'AC';
            }
            
            if (keyCode) {
                event.preventDefault();
                sendKey(keyCode, keyText);
            }
        });
        
        // 页面加载完成后聚焦，确保可以接收键盘事件
        window.addEventListener('load', () => {
            document.body.focus();
        });
    </script>
</body>
</html>
"""


@app.route('/')
def index():
    """主页面"""
    return render_template_string(HTML_TEMPLATE)


@app.route('/send_key', methods=['POST'])
def send_key():
    """接收按键并发送到FIFO"""
    try:
        data = request.get_json()
        key_code_name = data.get('key_code')
        key_text = data.get('key_text')
        
        # 将字符串转换为KeyCode枚举
        try:
            key_code = KeyCode[key_code_name]
        except KeyError:
            return jsonify({'success': False, 'error': f'Invalid key code: {key_code_name}'})
        
        # 发送到FIFO
        success = fifo_writer.send_key(key_code)
        
        return jsonify({
            'success': success,
            'key_code': key_code.value,
            'key_text': key_text
        })
        
    except Exception as e:
        print(f"Error in send_key: {e}")
        return jsonify({'success': False, 'error': str(e)})


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
