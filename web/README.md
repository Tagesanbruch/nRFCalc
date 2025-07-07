# CASIO fx-991ES PLUS Web Simulator

这是CASIO fx-991ES PLUS计算器的Web模拟器，用于与Zephyr计算器应用程序通信。

## 目录结构

```
web/
├── app.py                    # Flask主应用程序
├── templates/                # HTML模板
│   └── casio_fx991.html     # 计算器界面模板
├── static/                   # 静态资源
│   ├── css/
│   │   └── calculator.css   # 计算器样式
│   └── js/
│       └── calculator.js    # 计算器交互逻辑
└── README.md                # 本文件
```

## 功能特性

- **完整的CASIO fx-991ES PLUS布局**: 严格按照真实计算器设计
- **SHIFT/ALPHA模式支持**: 完整的二级功能按键支持
- **键盘快捷键**: 支持物理键盘输入
- **实时通信**: 通过FIFO与Zephyr应用程序实时通信
- **模块化设计**: HTML、CSS、JavaScript分离，便于维护

## 运行方式

```bash
# 从项目根目录运行
make run-keypad

# 或者直接运行
cd web
python app.py
```

## 技术栈

- **后端**: Flask (Python)
- **前端**: HTML5 + CSS3 + JavaScript
- **通信**: FIFO (命名管道)
- **字体**: Courier New (模拟LCD显示)

## 按键映射

所有按键都按照真实CASIO fx-991ES PLUS的布局和功能进行映射，包括：

- 数字键 (0-9)
- 运算符 (+, -, ×, ÷, =)
- 科学函数 (sin, cos, tan, log, ln, √)
- 特殊功能 (SHIFT, ALPHA, MODE, MATRIX, VECTOR)
- 括号和常数 (π, e)

## 开发说明

### 添加新按键

1. 在 `app.py` 的 `KeyCode` 枚举中添加新的键码
2. 在 `templates/casio_fx991.html` 中添加对应的HTML按钮
3. 在 `static/css/calculator.css` 中添加样式（如需要）
4. 在 `static/js/calculator.js` 中添加处理逻辑（如需要）

### 样式自定义

所有样式都在 `static/css/calculator.css` 中定义，包括：
- 按键颜色分类
- SHIFT/ALPHA标签样式
- 显示屏样式
- 响应式布局

### 交互逻辑

JavaScript代码 (`static/js/calculator.js`) 处理：
- 按键点击事件
- SHIFT/ALPHA模式切换
- 键盘快捷键
- 与后端的AJAX通信
