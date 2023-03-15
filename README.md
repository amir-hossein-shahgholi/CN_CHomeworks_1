# CN-CA1🌐
## Computer Network CA1. 📡

### Amir-Hossein Shahgholi (SID: 810199441)🎓

### Erfan-Soleymani (SID: 810199439)🎓
</br>

- [Setup Server](#Setup-server)
- [Client Side](#Client-side)
- [List of Errors](#List-of-Errors)
- [License](#license)

For generating `client.out` and `server.out` files run this command: 
<pre>
make
</pre>

## Setup-server</br>
then you have to run server side first.
<pre>
./server.out
</pre>
Then you should set time with command `setTime`.
</br>Ex. (Pay attention to the date format)
<pre>
setTime 27-02-2023
</pre>

Then server will start.
</br></br>

## Client-side</br>
You can run client file in another terminal
<pre>
./client.out
</pre>

Now you connect to the server.</br>
Options: </br>
`signup`: Ex.
<pre>
signup "Username"
</pre>
`signin`: Ex.
<pre>
signin "Username" "Password"
</pre>

If you signin or signup successfully you will see [Menu of Commands](#Commands-menu).

## Commands-menu
0. Logout
1. View user information
2. view all users
3. View rooms information
4. Booking
5. Canceling
6. pass day
7. Edit information
8. Leaving room
9. Rooms

## List-of-Errors
![alt text](./images/errors.png)

