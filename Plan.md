### 📝 总体开发计划

我们将项目分为几个逻辑清晰的阶段，逐步实现你的科学计算器。

#### Phase 0: 结构重构与抽象 (代码准备)

这一步的目标是把你现在的 `main.c` 变得整洁，为后续开发打下良好基础。你说到的“函数外移”正是此意。

1.  **创建显示驱动封装层 (`display_engine.c` / `.h`)**:

      * **目的**: 将所有与 `display_dev` 直接交互的底层代码（如 `display_write`, `display_blanking_off` 等）隔离起来。你的主逻辑不应该关心具体的像素格式或缓冲区细节。
      * **接口**: 在 `.h` 文件中定义清晰的、高层的绘图函数：
        ```c
        void display_engine_init(void);
        void display_engine_clear(void);
        void display_engine_draw_text(int x, int y, const char *str);
        void display_engine_draw_rect(int x, int y, int w, int h, uint32_t color);
        void display_engine_present(void); // 用于一次性更新屏幕
        ```
      * **实现**: 在 `.c` 文件中，`display_engine_init` 获取设备句柄并探测像素格式。`draw_text` 等函数内部处理像素转换和缓冲区操作，最终调用底层的 `display_write`。

2.  **创建按键处理模块 (`keypad_handler.c` / `.h`)**:

      * **目的**: 建立一个统一的按键输入来源。无论按键是来自串口、模拟器窗口还是最终的硬件GPIO，主逻辑都通过同一个接口获取输入。
      * **接口**:
        ```c
        typedef enum { KEY_NONE, KEY_0, KEY_1, /*...*/, KEY_PLUS, KEY_EQUAL, KEY_FN } key_code_t;
        void keypad_init(void);
        key_code_t keypad_get_key(void); // 非阻塞或阻塞方式获取按键
        ```

3.  **实现简易字体渲染**:

      * 你的 `display_engine_draw_text` 需要字体数据才能工作。
      * 最简单的方法是找一个开源的**点阵字体（bitmap font）**，比如 5x7 或 8x8 的。它就是一个 `uint8_t` 数组，每个字符用几个字节的位信息来表示字形。
      * `draw_text` 函数的逻辑就是：遍历输入字符串，根据每个字符的ASCII码去字体数组里找到对应的字形数据，然后一个像素一个像素地绘制到屏幕缓冲区。

-----

#### Phase 1: 核心逻辑与状态机 (计算器大脑)

1.  **定义计算器状态机 (`calculator_state_t`)**:

      * 根据你的经验，这很直接。至少需要以下状态：
        ```c
        typedef enum {
            STATE_INPUT_NORMAL,  // 正常输入表达式
            STATE_SHOW_RESULT,   // 显示计算结果
            STATE_SHOW_ERROR,    // 显示错误信息
            STATE_MENU_MODE,     // (可选) 模式选择或函数菜单
        } calculator_state_t;
        ```

2.  **定义计算器数据结构**:

      * 创建一个 `struct` 来管理计算器的所有数据：
        ```c
        struct calculator {
            calculator_state_t state;
            char input_buffer[64];  // 存放用户输入的表达式字符串
            int input_pos;
            char result_buffer[64]; // 存放计算结果的字符串
            // ... 其他需要的变量
        };
        ```

3.  **主循环 (`main` 函数)**:

      * 你的 `main` 函数现在会变得非常清晰：
        ```c
        void main(void) {
            // 初始化
            struct calculator calc = {0};
            display_engine_init();
            keypad_init();

            while(1) {
                // 1. 获取输入
                key_code_t key = keypad_get_key();

                // 2. 更新状态和数据 (处理按键)
                update_calculator_state(&calc, key);

                // 3. 渲染UI
                render_ui(&calc);

                k_msleep(20); // 避免CPU空转
            }
        }
        ```

-----

### ❓ 关键问题解答

#### 1\. 如何控制显示区域？通过字符串buffer？

**完全正确。** 你的核心就是维护 `input_buffer` 和 `result_buffer` 这两个字符串。

  * 在 `render_ui` 函数中，你会根据当前状态 `calc.state` 来决定画什么。
  * 如果状态是 `STATE_INPUT_NORMAL`，你就调用 `display_engine_draw_text(10, 20, calc.input_buffer)`。
  * 如果状态是 `STATE_SHOW_RESULT`，你就调用 `display_engine_draw_text(10, 40, calc.result_buffer)`。
  * 显示区域的控制，就是通过 `draw_text` 函数的 `x` 和 `y` 坐标参数实现的。你完全掌控着在屏幕的哪个位置画哪个字符串。

#### 2\. 如何模拟 Casio 计算器那么多的按键？

这是最有趣的部分！你的直觉非常准——**再套一层SDL软件**。`native_sim` 已经链接了SDL库用于显示，我们可以利用这个便利再创建一个窗口用于按键模拟。

**实现方案：**

1.  **创建一个独立的按键模拟线程**：在你的 `main.c` 中，使用 `K_THREAD_DEFINE` 创建一个专门处理SDL输入的新线程。

2.  **在这个新线程中编写SDL代码**：

      * 初始化SDL视频 (`SDL_Init(SDL_INIT_VIDEO)`)。
      * 使用 `SDL_CreateWindow` 和 `SDL_CreateRenderer` 创建第二个窗口，作为你的按键面板。
      * 在这个窗口里，循环绘制出所有按键的矩形和标签（"7", "8", "+", "sin" ...）。
      * 在主循环中，使用 `SDL_PollEvent` 来捕获鼠标事件。

3.  **搭建桥梁 (IPC通信)**：当SDL线程检测到鼠标点击了某个按键的区域时，它需要通知主计算器线程。**Zephyr的消息队列 (`k_msgq`)** 是实现这个“桥梁”的完美工具。

      * **定义消息**: `struct key_msg { key_code_t key; };`
      * **初始化**: 在 `main` 中 `k_msgq_init` 一个消息队列。
      * **发送**: SDL线程在检测到点击“7”键后，填充消息 `msg.key = KEY_7;`，然后调用 `k_msgq_put(&my_msgq, &msg, K_NO_WAIT);` 将消息发送出去。
      * **接收**: 之前定义的 `keypad_get_key()` 函数的内部实现就变成了从这个消息队列里接收消息：`k_msgq_get(&my_msgq, &msg, K_MSEC(10));`。

**这样做的好处**:

  * **图形化，直观**：你可以完美复刻Casio计算器的按键布局。
  * **解耦**：输入模拟（SDL线程）和计算器核心逻辑（主线程）完全分离。你的计算器逻辑不关心按键是来自SDL鼠标点击还是未来的物理按键。

-----

### Phase 2 -\> 3: 实现计算与移植

1.  **实现运算逻辑**: 这一步你已经很熟悉了。在 `update_calculator_state` 中，当按下“=”键时，调用你的表达式求值函数（如调度场算法 `Shunting-yard`）处理 `input_buffer`，并将结果格式化到 `result_buffer` 中，最后切换状态到 `STATE_SHOW_RESULT`。

2.  **移植到nRF52840硬件**:

      * **显示**: `display_engine` 基本不用改，因为 `DEVICE_DT_GET` 会自动获取你在 `overlay` 文件中定义的ST7789屏幕设备。
      * **按键**: 这是唯一需要大改的地方。你需要修改 `keypad_handler.c`：
          * 使用 `#ifdef CONFIG_ARCH_POSIX` ... `#else` ... `#endif` 来分隔代码。
          * 在 `#else` 部分（硬件代码），你不再创建SDL线程。而是使用 Zephyr 的 GPIO API (`gpio_pin_interrupt_configure`, `gpio_add_callback`) 或键盘扫描矩阵驱动 (`kscan`)。
          * 当物理按键中断发生时，中断服务程序（ISR）或回调函数会**向同一个消息队列 `my_msgq` 发送按键消息**。

看到妙处了吗？因为我们提前用消息队列做了抽象，你的主应用逻辑**一行代码都不用改**，就能从SDL模拟输入无缝切换到硬件GPIO输入。