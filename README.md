# CaNS - Battleship Bots

This is an implementation of the Battleship Bot for Computer and Network Systems at the University of the West of England.

## Features
* Console User Interface
* ARP Spoofing
* Packet Manipulation
* Allies
⋅⋅⋅* There is a flag, `isLeader`, which can be toggled to change the leadership of the alliance (results as default movement when all allies are together)
⋅⋅⋅* Supports upto 3 allies, more can be added by changing `MAX_ALLIES` and then updating the function `setupAllies to hold more data (if above the default of 3)
⋅⋅⋅* Will use default tactics until all allies have initialised their connections (sent you a packet with data)
⋅⋅⋅* Uses an encrypted xor flag against the bot's current X and Y coordinates.
* Uses structs instead of default arrays (no longer uses `shipX`, `shipY`, `shipHealth`, `shipFlag`, `shipType`, etc...)
⋅⋅⋅* Gives the opportunity to use "std::sort(...)" to sort through our arrays (defined in BattleshipBot.cpp)
* Good tactics
⋅⋅⋅* If playing solo, avoid any pack (unless the bot deems it safe to try and attack). A pack is defined as a ship of 2 or more.
⋅⋅⋅* The bot will stay on the edge of enemies to try and avoid getting hit whilst also shooting at the enemy.
⋅⋅⋅* If there are allies, it will default it's movement to find the average of how many allies are active however if any enemy is in sight, that will become the top priority.
⋅⋅⋅* On the fly flag changing, if we're not within distance of being attacked by an ally, change our flag to our closest enemy to try and become their "friend".


## Authors

* **Reece Benson** [Website](http://reecebenson.me/) | [GitHub](https://github.com/reecebenson)

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for more details.
