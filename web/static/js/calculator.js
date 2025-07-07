// CASIO fx-991ES PLUS Calculator JavaScript

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
