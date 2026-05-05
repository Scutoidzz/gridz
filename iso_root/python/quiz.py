# title: Math Quiz
from gridz_io import input
import random

print("=== Math Quiz ===")
score = 0
for i in range(5):
    a = random.randint(1, 10)
    b = random.randint(1, 10)
    ans = input(str(a) + " + " + str(b) + " = ")
    if int(ans) == a + b:
        print("Correct!")
        score += 1
    else:
        print("Wrong (" + str(a + b) + ")")
print("Score: " + str(score) + "/5")
