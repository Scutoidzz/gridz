# title: Guess the Number
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
        print("Correct in " + str(guesses) + " guesses!")
        break
    elif guess < number:
        print("Too low")
    else:
        print("Too high")
