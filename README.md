# WebServer
Another webserver.

## Features
Here is a quick summary of some of the features *supported* by this server (italics means: an upcoming feature):
* OCSP Stapling
* Simple configuration
* HTTP/1.1
* HTTP/2
* _HTTP/3_
* _Caching Mechanisms_
* _Automatic Certificates_
* _

## Status
This project is currently in a very early stage, and a lot of unit/fuzztesting has to be done before actual production. This is a hobbyist project, and any contributions are welcome.

## Goal
The goal was to create a modern and fast web server, with a low memory footprint. The latter is so you can run this server on many systems.

## Requirements
* C89 Compiler
* OpenSSL(-dev)
* A POSIX-compliant operating system (e.g. Linux, BSD)

## License
The source code of this webserver is licensed under the permissive (for the direct user) license as stated in the COPYING file (this license is a.k.a. the libpng/zlib license).
Despite the permissive nature of this license, I still encourage any direct users to respect other users' freedom and privacy.
