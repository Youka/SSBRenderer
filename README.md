# SSBRenderer
<table border=0><tr>
<td><img src=src/res/logo.bmp /></td>
<td><b>SSBRenderer</b> is a <a href=http://en.wikipedia.org/wiki/Plug-in_%28computing%29>plugin</a> for <a href=http://en.wikipedia.org/wiki/Frameserver>frameservers</a> on <a href=http://en.wikipedia.org/wiki/Microsoft_Windows>Windows</a> & <a href=http://en.wikipedia.org/wiki/Unix>Unix</a> platforms and <a href=http://en.wikipedia.org/wiki/C_%28programming_language%29>C</a> library to <a href=http://en.wikipedia.org/wiki/Rendering_%28computer_graphics%29>render</a> 2D graphics by SSB (<i>Substation Beta</i>) data on <a href=http://en.wikipedia.org/wiki/Film_frame>frames</a> of video <a href=http://en.wikipedia.org/wiki/Streaming_media>streams</a>.<br>High performance for softsubbing, powerful styling properties and usability have the highest priority in development.</td>
</tr></table>

### Build
Dependencies:
* Windows: already precompiled static libraries in "src/libs/win32" & "src/libs/win64"
* Unix: <a href=http://muparser.beltoforion.de/>muParser</a> <a href=http://www.pango.org/>pango</a> <a href=http://cairographics.org/>cairo</a>
* Debian/Ubuntu: libmuparser-dev libpango1.0-dev libcairo2-dev

Currently there're 2 building ways:
* Use Code::Blocks to open project file <b>SSBRenderer.cbp</b>. Select your build and compile.
* Execute <b>Makefile</b> with options (BUILD=debug | clean | install | uninstall). On Unix run <b>./configure</b> first, before you start the Makefile.

### Example
See [examples](examples) or seach online for user videos & scripts.

### Contact
Just whisper <b>Youka</b> on <a href="http://en.wikipedia.org/wiki/IRC">IRC</a> servers [freenode](https://www.freenode.net/) or [rizon](http://rizon.net/), open an issue on github or write a PM in [doom9 forum](http://forum.doom9.org/member.php?u=197060).

### Contributing
Every help is welcome, just contact me and tell me what you want to do.<br>Tutorials, more detailed documentations, bug fixes, source code improvements, ideas for the future, ...

### License
This software is under zlib license. For details, read <b>LICENSE.txt<b>.
