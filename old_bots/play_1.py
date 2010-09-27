# for continuous play on www.benzedrine.cx/planetwars

# EDIT the "cmd" string with the command to run your bot on the tcp server
# command to run tcp server with your own bot
#  (ex: "/home/me/tcpserver/tcp 213.3.30.106 9999 supaman314 /home/me/MyBot" )
cmd = "tcp.exe 72.44.46.68 995 iouri.1 -p victory iouri_timeline_seeker_3"

import os
import sys
import time
import random
import subprocess

# global vars
numRounds = 0
numWins = 0
numLosses = 0
numDraws = 0

def doRound():
    global numRounds
    global numWins
    global numLosses
    global numDraws
    print("-----------------------------------------------")
    print("------------------- Round " + str(numRounds) + " -------------------")
    print("Wins: " + str(numWins) + ",  Losses: " + str(numLosses) \
            + ",  Draws: " + str(numDraws))
    # connect to server (and thus initiate match)
    #p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, \
    #                     stderr=subprocess.STDOUT)
    p = os.popen(cmd)
    print("Playing Match...")
    while True:
        line = p.readline()
        if not line:
            break;
        if line.startswith("You currently"):
            print("Current Elo: " + line.split()[3])
        if line.startswith("Your map is"):
            print("Playing map: " + line.split()[3])
        if line.startswith("Your opponent"):
            print("Opponent: " + line.split()[3] + "  (" + line.split()[5] + \
                  " elo)")
        if line.startswith("You LOSE"):
            print("You LOSE this match")
            numLosses += 1
        if line.startswith("You WIN"):
            print("You WIN this match")
            numWins += 1
        if line.startswith("You DRAW"):
            print("You DRAW this match")
            numDraws += 1
        sys.stdout.flush()

    print("done")

def sleepAwhile(): # to prevent pairing up with same opp over and over
    duration = random.randrange(2,30) # 35 to 125 seconds
    print("press Ctrl+C to quit now while not in a match")
    print("sleeping a few seconds between matches.")
    for i in range(1, duration+1):
        time.sleep(1)
    print("")    
        
        
if __name__ == "__main__":
    print("\t\t\treplay.py \n")
    try:
        while True:
            numRounds += 1
            doRound()
            sleepAwhile()

            
    except KeyboardInterrupt:
        print("\n\nQuitting.")
        os._exit(0)
