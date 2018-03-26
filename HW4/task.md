## CSCI 232 Operating Systems

### Homework 4 -- Quiz server

Consider a server that can give a predefined synchronized quiz to a group. For each question, the first person to
answer the question correctly is recorded as the winner, and gets 2 points. Other users who get the correct answer
get 1 point, no answer gets 0 points, and the wrong answer yields -1 points.

When the server has no clients, the first client to connect is considered the group leader, and must specify the
number of users who will take the quiz, so that they start together. Every client, including the group leader, must
supply a name.

Once the quiz begins, the multiple choice questions (from the server's input file) are presented, one by one, until
the quiz is finished. Each client is required to answer a question in a reasonable time, or send a no-answer
message. Assume clients are well-behaved in this sense, to eliminate the need for timers (in this version at least).

The only bad behavior you must anticipate from clients is that they may exit at any time, so the group may get
smaller over time, and that's ok, until the last client leaves (of course). Any clients that arrive after the initial
desired group size was reached should be rejected (even if the group has gotten smaller).

Once all connected clients have answered a question, the server should tell all clients the name of the winner, and
then proceed immediately to the next question. After the final question is asked, the server should return the full
quiz results, and then close the connections with the players, and be ready to start another quiz. If all clients
disconnect before the quiz is finished, the server also must be ready to start again with another group leader.

For this homework, you may use telnet on the client side, but we will also test your server with a client that really
produces some simultaneous accesses. Again, you may assume very well-behaved clients: all messages will be
well-formatted. The maximum group size a client can request is 1010, basically just less than the max open file
descriptors (system constant `FD_SETSIZE`, whose normal value is 1024). Assume that all clients who remain
connected will answer questions within a reasonable amount of time. The only thing you cannot count on is that
clients remain connected, or that they come and go in any defined order.

There are of course ways to solve this problem without using threads, or with a mixture of multiplexing and
threads, and so on. However, the purpose of this homework is to practice synchronizing threads. Therefore, it is
__required__ that once a client connects, a thread is launched to handle the interaction on that socket, and that thread
stays active until that socket is closed, and no other threads read or write on that socket. Do no reads or writes on
the main thread. Working solutions that do not abide by this requirement __will not be accepted__.

_As always, any electronic copying from one another will result in a zero on the assignment (and an F in the class
if this is the second time). Any copying from other sources must be clearly and completely cited: it must be clear
exactly what section of code came from exactly where. Violations of this rule will result in the same consequences
as for copying from another person._

#### 1. The input file of quiz questions

Have the server require a new argument, a filename. The filename must be first, then the optional port number is
second. This file will store questions and answers for a quiz. It may be read using normal C library functions,
such as `fgets`. You may assume the file, if it exists and is readable, is well-formatted. The format is simple
and doesn’t really require complex parsing. Each question has an id and is on its own line, followed by the possible
answers, one per line, also identified somehow. The last answer line is followed by a blank line. Then the id of
the correct answer appears on its own line, followed a blank line. The total number of characters in a question
with all its answers, fully formatted, including newlines, will not exceed 2048 bytes.


Quiz file format:
```
id question \n
id answer \n
id answer \n
id answer \n
\n
id \n
\n
```

Example:
```
1 What is a system call?
A. A call to the system
B. A function call that invokes the operating system to take some action
C. A call to the operating system

B

2 Another question
    (and so on)
```


####2. The protocol

Each protocol command must end with a CR (`\r`) and LF (`\n`) except where noted. The combination helps to
simply handle input from human users on various OSes.

When the number of connected clients increases from 0 to 1:
Server welcomes the group leader. The group leader sends its name and desired group size.
```
Server sends:       Client answers:
QS|ADMIN            GROUP|Lname|Lnum
WAIT
```

While the number of connected clients is less than the desired group size:
Server welcomes a prospective group member. The client sends its name.
```
Server sends:       Client answers:
QS|JOIN             JOIN|Cname
WAIT
```

If the group has already been formed, or the quiz has already started, or a leader has connected but not defined
the group size yet:
```
Server sends:       Client answers:
QS|FULL             Nothing because server closes the socket
```

#####Protocol for sending questions and announcing a winner.

As long as at least one client remains in the group, keep asking questions, until all questions are exhausted. When
they are all finished, announce the winner (and standings of all players), and disconnect from the clients.

The server sends the size of the entire question plus answers string, and then the string. A terminating `CRLF` is
not used or required because the size precedes the message, and the message contains newlines within it. If the
client does not know the answer it sends the special text `NOANS` in place of the answer ID.

After all the users have answered, the server sends the client the winner’s name (the name of the client who scored
2 for the fastest right answer). This message is immediately followed by the next question.
```
Server sends:                   Client answers:
QUES|size|full-question-text    ANS|answerID
WIN|name
```

After all the questions are finished, the server sends the quiz standings as a series of names and scores in
descending order by score, terminated by CRLF. Every client socket is closed, and the server is ready to start
again with a new leader.
```
Server sends:                       Client answers:
RESULT|name|score|name|score ...    Nothing because server closes the socket
```
