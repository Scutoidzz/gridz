# Python Games for Gridz Terminal

Once the Python terminal app is working, you can run these games by typing them directly or saving them to files.

## Using `input()` with Gridz

The Python terminal provides `input()` through the `gridz_io` module:

```python
from gridz_io import input

name = input("What's your name? ")
print(f"Hello, {name}!")
```

Or use the built-in `input()` directly (when available).

---

## Interactive Games (with input())

## Game 1: Number Guessing Game (with input)

```python
from gridz_io import input
import random

print("=== Guess the Number ===")
number = random.randint(1, 100)
guesses = 0

while True:
    guess_str = input("Enter guess (1-100): ")
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

## Game 2: Rock, Paper, Scissors

```python
from gridz_io import input
import random

print("=== Rock, Paper, Scissors ===")

choices = ["rock", "paper", "scissors"]
computer = choices[random.randint(0, 2)]

player_choice = input("Rock, paper, or scissors? ").lower()

print(f"Computer: {computer}")
print(f"You: {player_choice}")

if player_choice == computer:
    print("Tie!")
elif (player_choice == "rock" and computer == "scissors") or \
     (player_choice == "paper" and computer == "rock") or \
     (player_choice == "scissors" and computer == "paper"):
    print("You win!")
else:
    print("Computer wins!")
```

## Game 3: Higher or Lower

```python
from gridz_io import input
import random

print("=== Higher or Lower ===")
number = random.randint(1, 100)
guesses = 0

while True:
    try:
        guess_str = input("Guess: ")
        guess = int(guess_str)
        guesses += 1
        
        if guess < number:
            print("Higher")
        elif guess > number:
            print("Lower")
        else:
            print(f"Correct! ({guesses} guesses)")
            break
    except:
        print("Invalid input")
```

## Game 4: Simple Math Quiz

```python
from gridz_io import input
import random

print("=== Math Quiz ===")
score = 0

for i in range(5):
    a = random.randint(1, 10)
    b = random.randint(1, 10)
    
    answer_str = input(f"{a} + {b} = ")
    answer = int(answer_str)
    
    if answer == a + b:
        print("Correct!")
        score += 1
    else:
        print(f"Wrong (answer: {a + b})")

print(f"Score: {score}/5")
```

## Game 5: Adventure Text Game

```python
from gridz_io import input

print("=== Adventure ===")
print("You wake in a dark room.")

choice = input("Go left or right? ").lower()

if choice == "left":
    print("You find a treasure chest!")
    print("You win!")
elif choice == "right":
    print("You encounter a monster!")
    print("You lose!")
else:
    print("You stand confused.")
```

---

## Non-Interactive Games (copy-paste friendly)

## Game 1: Number Guesser (AI vs Player)

```python
import random

print("=== AI Number Guesser ===")
print("I'm thinking of a number 1-100")
print("")

# The AI picks a number
number = random.randint(1, 100)

# Simulate some guesses (since input() not yet fully implemented)
guesses = [50, 75, 25, 60, 55]
for i, guess in enumerate(guesses):
    print(f"Guess {i+1}: {guess}")
    if guess == number:
        print(f"Correct! The number was {number}")
        break
    elif guess < number:
        print("Too low!")
    else:
        print("Too high!")
```

## Game 2: Dice Roller Simulator

```python
import random

print("=== Dice Roller ===")
print("")

for roll in range(6):
    die1 = random.randint(1, 6)
    die2 = random.randint(1, 6)
    total = die1 + die2
    print(f"Roll {roll+1}: {die1} + {die2} = {total}")
```

## Game 3: Lottery Ticket

```python
import random

print("=== Lottery ===")
print("Your lottery numbers:")

# Generate 6 random numbers 1-49
for i in range(6):
    num = random.randint(1, 49)
    print(f"  {num}")

print("")
print("Winning numbers:")
for i in range(6):
    num = random.randint(1, 49)
    print(f"  {num}")
```

## Game 4: Coin Flip Streak

```python
import random

print("=== Coin Flip Streak ===")
print("")

max_streak = 0
current_streak = 0
last_flip = ""

for i in range(20):
    flip = "Heads" if random.randint(1, 2) == 1 else "Tails"
    print(f"Flip {i+1}: {flip}")
    
    if flip == last_flip:
        current_streak += 1
    else:
        current_streak = 1
    
    if current_streak > max_streak:
        max_streak = current_streak
    
    last_flip = flip

print(f"Longest streak: {max_streak}")
```

## Game 5: Simple Card Shuffle

```python
import random

print("=== Card Shuffle ===")
print("")

# Deck: 1-13 = Ace-King of each suit
cards = list(range(1, 53))

# Shuffle
for i in range(len(cards)):
    j = random.randint(0, len(cards)-1)
    cards[i], cards[j] = cards[j], cards[i]

# Show first 5 cards
print("Top 5 cards:")
for i in range(5):
    suit = ["Hearts", "Diamonds", "Clubs", "Spades"][cards[i] // 13]
    rank = ["A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"][cards[i] % 13]
    print(f"  {rank} of {suit}")
```

## Game 6: Monster Battle (Text)

```python
import random

print("=== Monster Battle ===")
print("")

player_hp = 20
monster_hp = 15
turn = 1

while player_hp > 0 and monster_hp > 0:
    print(f"Turn {turn}: Player HP={player_hp}, Monster HP={monster_hp}")
    
    # Player attack
    damage = random.randint(1, 6)
    monster_hp -= damage
    print(f"  You hit for {damage} damage!")
    
    if monster_hp <= 0:
        print("Victory! You defeated the monster!")
        break
    
    # Monster attack
    damage = random.randint(1, 4)
    player_hp -= damage
    print(f"  Monster hits for {damage} damage!")
    
    if player_hp <= 0:
        print("Defeat! You were slain.")
        break
    
    print("")
    turn += 1

print("Battle Over!")
```

---

## What's Implemented

✅ **`input()` via `gridz_io` module** — Use `from gridz_io import input` for interactive games
✅ **`random.randint()`** — Generate random numbers
✅ **`print()`** — Output text
✅ **Keyboard input** — Type responses to `input()` prompts
✅ **Basic Python** — Variables, loops, conditionals, functions

## Current Limitations

- **File loading** — Can't load `.py` files yet (copy-paste into terminal)
- **Limited modules** — Only `random` and `gridz_io` available
- **~100 chars per line** — Terminal wraps long text
- **No exceptions** — Try/except works but exception details limited
- **No file I/O** — Can't read/write files to disk yet

## Future Features

To add full filesystem support:
1. Integrate FAT32 filesystem access into `files` module
2. Allow `open()` and `read()` for loading game files
3. Add game save/load functionality

## Testing

To test interactive games, open the Python app and try:

```python
from gridz_io import input

name = input("Enter name: ")
print(f"Hello, {name}!")
```

Then try any of the interactive games above!
