chat-server.c is a program I wrote to create a simple, functional chat server using HTTP-like request handling. It allows users to post messages, add reactions to messages, and retrieve the chat log. It relies on http-server.c for lower-level server operations and socket management. Should catch any user mistakes and throw corresponding errors. I programmed a URL decoder which takes in special characters as %HEX format, for example, %20 is a space and %F0%9F%91%8D is 👍

```bash
# How to use
make
./chat-server [port]

# Then you can use these to interact:

# posting
curl "http://localhost:8080/post?user=[USERNAME]&message=[MESSAGE]" 

# reacting
curl "http://localhost:8080/react?user=[USERNAME]&message=[MESSAGE]&id=[ID_NUMBER]"

# retrieve chats
curl "http://localhost:8080/chats"

# reset chat log
curl "http://localhost:8080/reset"
```