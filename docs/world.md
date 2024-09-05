# World Documentation

The world setup in APE Tech works a little differently to traditional 3D game engines. Rather than having your global coordinate space, instead, each room in the world has its own coordinate space, and the world is merely a container that tracks all the rooms.

Depending on where the camera is, everything is then rendered from

Each object can have a collection of children under it that moves with it.
