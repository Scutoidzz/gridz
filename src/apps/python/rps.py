# title: Rock Paper Scissors
from gridz_io import input
import random

print("=== Rock Paper Scissors ===")
choices = ["rock", "paper", "scissors"]
comp = choices[random.randint(0, 2)]
you = input("rock/paper/scissors? ").lower()
print("Computer: " + comp)
print("You: " + you)
if you == comp:
    print("Tie!")
elif (you == "rock" and comp == "scissors") or \
     (you == "paper" and comp == "rock") or \
     (you == "scissors" and comp == "paper"):
    print("You win!")
else:
    print("Computer wins!")
