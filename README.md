#### Screenshots
<table>
  <tr>
    <td><img src="/.readme/s1.png"></td>
    <td><img src="/.readme/s2.png"></td>
  </tr>
  <tr>
    <td><img src="/.readme/s3.png"></td>
    <td><img src="/.readme/s4.png"></td>
  </tr>
</table>

### Building:
#### Windows:
1. `git clone https://github.com/uowuo/abaddon && cd abaddon`
2. `vcpkg install gtkmm:x64-windows nlohmann-json:x64-windows ixwebsocket:x64-windows zlib:x64-windows simpleini:x64-windows sqlite3:x64-windows openssl:x64-windows curl:x64-windows`
3. `mkdir build && cd build`
4. `cmake -G"Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=c:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVCPKG_TARGET_TRIPLET=x64-windows ..`
5. Build with Visual Studio  
  
Or, do steps 1 and 2, and open CMakeLists.txt in Visual Studio if `vcpkg integrate install` was run

#### Mac:
1. `git clone https://github.com/uowuo/abaddon && cd abaddon`
2. `brew install gtkmm3 nlohmann-json`
3. `mkdir build && cd build`
4. `cmake ..`
5. `make`

#### Linux:
1. Install dependencies: `libgtkmm-3.0-dev`, `libcurl4-gnutls-dev`, and [nlohmann-json](https://github.com/nlohmann/json)
2. `git clone https://github.com/uowuo/abaddon && cd abaddon`
3. `mkdir build && cd build`
4. `cmake ..`
5. `make`

### Downloads (from CI):
- Windows: [here](https://nightly.link/uowuo/abaddon/workflows/ci/master/build-windows-RelWithDebInfo.zip)
- MacOS: [here](https://nightly.link/uowuo/abaddon/workflows/ci/master/build-macos-RelWithDebInfo.zip) unsigned, unpackaged, requires gtkmm3 (e.g. from homebrew)
- Linux: [here](https://nightly.link/uowuo/abaddon/workflows/ci/master/build-linux-MinSizeRel.zip) unpackaged (for now), requires gtkmm3. built on Ubuntu 18.04 + gcc9  

Make sure you start from the directory where `css` and `res` are

#### Dependencies:  
* [gtkmm](https://www.gtkmm.org/en/)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [IXWebSocket](https://github.com/machinezone/IXWebSocket)
* [libcurl](https://curl.se/)
* [zlib](https://zlib.net/)
* [simpleini](https://github.com/brofield/simpleini)
* [SQLite3](https://www.sqlite.org/index.html)

### TODO:
* Voice support
* Unread indicators
* User status
* Nicknames
* Server/user management
* A bunch of other stuff

### Styling
#### CSS selectors
.channel-list - Container of the channel list  
.channel-row - All rows within the channel container  
.channel-row-channel - Only rows containing a channel  
.channel-row-category - Only rows containing a category  
.channel-row-guild - Only rows containing a guild  
.channel-row-label - All labels within the channel container  
.nsfw - Applied to channel row label's and their container for NSFW channels  
  
.messages - Container of user messages  
.message-container - The container which holds a user's messages  
.message-container-author - The author label for a message container  
.message-container-timestamp - The timestamp label for a message container  
.message-container-extra - Label containing BOT/Webhook  
.message-text - The text of a user message  
.message-attachment-box - Contains attachment info  
.message-reply - Container for the replied-to message in a reply (these elements will also have .message-text set)  
.reaction-box - Contains a reaction image and the count  
.reacted - Additional class for reaction-box when the user has reacted with a particular reaction  
.reaction-count - Contains the count for reaction  
  
.completer - Container for the message completer  
.completer-entry - Container for a single entry in the completer  
.completer-entry-label - Contains the label for an entry in the completer  
.completer-entry-image - Contains the image for an entry in the completer  
  
.embed - Container for a message embed  
.embed-author - The author of an embed  
.embed-title - The title of an embed  
.embed-description - The description of an embed  
.embed-field-title - The title of an embed field  
.embed-field-value - The value of an embed field  
.embed-footer - The footer of an embed  
  
.members - Container of the member list  
.members-row - All rows within the members container  
.members-row-label - All labels in the members container  
.members-row-member - Rows containing a member  
.members-row-role - Rows containing a role  
.members-row-avatar - Contains the avatar for a row in the member list  
  
.status-indicator - The status indicator  
.online - Applied to status indicators when the associated user is online  
.idle - Applied to status indicators when the associated user is away  
.dnd - Applied to status indicators when the associated user is on do not disturb  
.offline - Applied to status indicators when the associated user is offline  
  
  
### Settings
Settings are configured (for now) by editing abaddon.ini  
This listing is organized by section.  
For example, memory_db would be set by adding `memory_db = true` under the line `[discord]`

#### discord
* memory_db (true or false, default false) - if true, Discord data will be kept in memory as opposed to on disk
* token (string) - Discord token used to login, this can be set from the menu
* prefetch (true or false, default false) - if true, new messages will cause the avatar and image attachments to be automatically downloaded

#### http
* user_agent (string) - sets the user-agent to use in HTTP requests to the Discord API (not including media/images)
* concurrent (int, default 10) - how many images can be concurrently retrieved

#### gui
* member_list_discriminator (true or false, default true) - show user discriminators in the member list
* emojis (true or false, default true) - resolve unicode and custom emojis to images. this needs to be false to allow GTK to render emojis by itself
* css (string) - path to the main CSS file
* animations (true or false, default true) - use animated images where available (e.g. server icons, emojis, avatars). false means static images will be used

#### misc
* linkcolor (string) - color to use for links in messages
