# title: Get candy at 3AM
from gridz_io import input
import random

print("=== Get Candy at 3AM ===")
print("The corner store is 4 blocks away. It's 2:59 AM.")
print("")
print("Do you accept the challenge?")
print("1. Accept the challenge")
print("2. I'm too chicken")

cursor = input("Choice? ")

if cursor == "1":
    print("You accept the challenge.")
elif cursor == "2":
    print("Be that way then.")
    print("Ending: Must be fun at parties")
    exit()
else:
    print("Yeah, I'm taking that as a yes.")

print("")
print("You wake up. Time: 2:59 AM")
print("A. Go back to sleep")
print("B. Unlock your phone")
print("C. Get up and walk to the store")
cursor = input("Do you: ")

if cursor == "A" or cursor == "a":
    print("You roll over and pull the blanket up.")
    print("Yo ahh is still hungry...")
    print("")
    print("Ending: Underachiever")
    exit()

elif cursor == "B" or cursor == "b":
    print("You unlock your phone.")
    print("There are 4 apps in your dock: TikTok, DoorDash, X, and YouTube.")
    print("")
    print("What do you pick?")
    print("A. TikTok")
    print("B. DoorDash")
    print("C. X")
    print("D. YouTube")
    app = input("App? ")

    if app == "B" or app == "b":
        print("You open DoorDash.")
        print("Minimum order: $10. Delivery fee: $4.99. Service fee: $3.49.")
        print("A single bag of gummy bears costs $1.89.")
        print("")
        print("A. Order anyway")
        print("B. Close the app")
        order = input("Choice? ")
        if order == "A" or order == "a":
            print("You spend $14.37")
            print("It arrives at 4:17 AM.")
            print("")
            print("Ending: Late delivery (should have drove yo ahh)")
            exit()
        else:
            print("You close DoorDash and lie there staring at the ceiling.")
            print("")
            print("Ending: Goofy")
            exit()
    else:
        print("twin, you fked up.")
        print("")
        print("Ending: Distracted")
        exit()

elif cursor == "C" or cursor == "c":
    print("You stand up and walk to the door")
    print("")
    print("A. Try the door (might be locked)")
    print("B. Give up")
    print("C. Look for your keys first")
    door = input("Do you: ")

    if door == "A" or door == "a":
        print("You try the door.")
        locked = random.randint(0, 1)
        if locked == 1:
            print("It's unlocked! You step outside.")
            print("It's cold.")
        else:
            print("Locked. Are u dumb.")
            door = "C" 

    if door == "B" or door == "b":
        print("You stare at the door for a long moment.")
        print("Then you go back to bed.")
        print("")
        print("Ending: Quitter")
        exit()

    if door == "C" or door == "c":
        print("You start searching the apartment.")
        print("")
        print("Where do you check first?")
        print("A. The kitchen counter")
        print("B. Your jacket pocket")
        print("C. The couch cushions")
        search = input("Check: ")

        if search == "A" or search == "a":
            print("That wasn't in there :uuh-cat:.")
            found = False
        elif search == "B" or search == "b":
            print("What were you expecting")
            found = True
        elif search == "C" or search == "c":
            print("You dig between the cushions and find the Keys.")
            found = True
        else:
            print("You stand there confused. Keys don't find themselves.")
            found = False

        if not found:
            print("No luck there. You try the jacket pocket.")
            print("There they are.")

        print("Keys in hand.")
        print("")
        print("You unlock the door and step outside.")

    # -- Outside --
    print("")
    print("some guy nextdoor is playing cupcakke mmusic")
    print("You've got 4 blocks to walk.")
    print("")
    print("A. Walk normally")
    print("B. Jog (it's cold)")
    print("C. Stop and stare at the sky for a bit")
    walk = input("You: ")

    if walk == "C" or walk == "c":
        print("You tilt your head back.")
        print("You stand there for two whole minutes.")
        print("Then you keep walking.")

    if walk == "B" or walk == "b":
        print("you jog")
        print("A police officer stops you and arrests you. the end")
        print("Ending: Jail")
    print("")
    print("You arrive at the corner store. The fluorescent light flickers inside.")
    print("The door sensor dings as you push through.")
    print("")
    print("The cashier barely looks up. Fair.")
    print("")
    print("What do you grab?")
    print("A. Gummy bears")
    print("B. Sour worms")
    print("C. Peanut butter cups")
    print("D. Grab one of everything")
    candy = input("Candy? ")

    if candy == "A" or candy == "a":
        candy_name = "gummy bears"
    elif candy == "B" or candy == "b":
        candy_name = "sour worms"
    elif candy == "C" or candy == "c":
        candy_name = "peanut butter cups"
    elif candy == "D" or candy == "d":
        candy_name = "everything you could carry"
        print("The cashier finally looks up.")
    else:
        candy_name = "something random off the shelf"
        print("You grabbed it without even looking. Bold.")

    print("")
    price = round(random.uniform(1.49, 6.99), 2)
    print("You pay $" + str(price) + " for the " + candy_name + ".")
    print("")
    print("You step back outside. The walk home feels shorter.")
    print("You eat some of it before you even get back inside.")
    print("")

    event = random.randint(1, 3)
    if event == 1:
        print("You meet the legendary scutoid.")
        print("but then he lowkey shoulder checks you.")
    elif event == 2:
        print("You see a QR code to see someone's meat. Scanning it leads you to a site selling steaks")
        print("You nod anyway.")
    else:
        print("M A R S H M A L L O W")

    print("")
    print("You get back home. You eat the rest of the " + candy_name + " in bed.")
    print("It wasn't that good.")
    print("")
    print("Ending: Mission Accomplished")
    exit()

else:
    print("Yeah, that's an A.")
    print("You go back to sleep.")
    print("")
    print("Ending: Underachiever")
    exit()
