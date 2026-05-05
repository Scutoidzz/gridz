# Python Terminal Demo & Tutorial

## Quick Start

1. Open the Python app from the start menu
2. Try these commands one by one:

```python
# Test 1: Basic print
print("Hello from Python!")

# Test 2: Random numbers
import random
print(random.randint(1, 100))

# Test 3: Variables and loops
for i in range(5):
    print(f"Number {i}")

# Test 4: Interactive input
from gridz_io import input
name = input("What is your name? ")
print(f"Hello, {name}!")

# Test 5: Game - Rock Paper Scissors
from gridz_io import input
import random
choices = ["rock", "paper", "scissors"]
comp = choices[random.randint(0, 2)]
you = input("rock/paper/scissors? ")
print(f"Computer: {comp}, You: {you}")
```

## Complete Game Example

Copy and paste this line-by-line for a complete number guessing game:

```python
from gridz_io import input
import random
print("=== Guess the Number ===")
number = random.randint(1, 100)
guesses = 0
while True:
    guess_str = input("Guess (1-100): ")
    guess = int(guess_str)
    guesses += 1
    if guess == number:
        print(f"Correct in {guesses} guesses!")
        break
    elif guess < number:
        print("Too low")
    else:
        print("Too high")
```

## Multi-line Python

For longer programs, you can:

1. **Type line-by-line** — Just press Enter after each line and the terminal continues accepting input
2. **Indentation** — Use spaces for function/loop bodies (Python requires it)
3. **Paste** — Copy an entire game from PYTHON_GAMES.md and paste it line-by-line

### Example: Multi-line Function

```python
def greet(name):
    print(f"Hello, {name}!")
    print("Welcome to Gridz!")

greet("Player")
```

Type it as:
```
> def greet(name):
>     print(f"Hello, {name}!")
>     print("Welcome to Gridz!")
> 
> greet("Player")
```

(The blank line tells Python the function is complete)

## Available Modules

### `random`
```python
import random
random.randint(1, 100)  # Random int between 1 and 100
```

### `gridz_io` (for input)
```python
from gridz_io import input
name = input("Your name: ")
```

### Built-in Functions
- `print()` — Output text
- `int()` — Convert to integer
- `str()` — Convert to string
- `len()` — String/list length
- `range()` — Generate sequence
- `list()` — Create list
- `dict()` — Create dictionary

## Common Patterns

### Simple Game Loop
```python
from gridz_io import input
import random

score = 0
while True:
    action = input("Play again? (y/n): ")
    if action.lower() == "n":
        break
    score += 1

print(f"Final score: {score}")
```

### Math Game
```python
from gridz_io import input
import random

correct = 0
for q in range(3):
    a = random.randint(1, 10)
    b = random.randint(1, 10)
    ans = input(f"{a}+{b}=? ")
    if int(ans) == a + b:
        correct += 1

print(f"Score: {correct}/3")
```

### Adventure Chooser
```python
from gridz_io import input

print("You see two doors...")
choice = input("Left or Right? ")

if choice.lower() == "left":
    print("You find gold!")
else:
    print("You meet a dragon!")
```

## Tips

- **Typos?** Press Backspace to fix them before pressing Enter
- **Stuck?** Close the window and reopen to reset
- **Want to save?** Copy the game code and paste it into a .py file (future feature)
- **Slow?** The REPL interprets each line, so it's expected to be slower than native Python

## Troubleshooting

**"NameError: name 'input' is not defined"**
- Use `from gridz_io import input` first

**"SyntaxError"**
- Check indentation (must use spaces)
- Make sure strings have matching quotes

**Game stops or freezes**
- The REPL may be waiting for more input
- Try pressing Enter to continue

**Can't paste long code?**
- Paste in smaller chunks (5-10 lines at a time)
- Use the PYTHON_GAMES.md examples as a guide
