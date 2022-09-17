# ComeTogether server
Service that implements back-end logic for ComeTogether application. 


# Description

## General idea
The main idea of the application is to allow people all around the world to create meetings on the map,
see meetings nearby, join other people\`s meetings, chat within secific meeting and with each other.

## Tech description
Upon successful login/registration by the means of Google JWT token the server saves the mapping `session_id` -> `user_id` in MongoDB, then the client should call `SubscribeOfEvents` gRPC method.
The method itself is a [server-streaming](https://grpc.io/docs/what-is-grpc/core-concepts/#server-streaming-rpc) one, so it returns a *stream* of events, such as:
 * Events actions (adding, remowing, editing)
 * New messages
 * Status updates - *Not supporetd*



The project is done as proof-of-concept for my course work and there is a lot of work to be done, though.

## Architecture

![Architecture](https://imgur.com/qYc1AAW.jpg)

## Techonoliges used
The communication between the parts of the project is done by the means of the gRPC protocol. The server uses mongocxx driver for interacting with MongoDB and store there details about the users, messages, current events.

## AI Service

Mainly, the AI service does the following two tasks:
* Validating the user upon registration
* Recomending events for user that he might be interested in (not really testes) 

[Link to Github repo](https://github.com/Piaiai/cometogether)

## Mobile apps

Unfortunately, mobile app both for Android and IOS have not been published yet. Hope this will change in future :)

## TODO
* Think out how to do real validation of the Google JWT token received from the mobile clients
* Rewrite tests with the using of GTest
* For now, the architecture is monolith; Think about how to split it
* Add validation for SubscribeToEvents





