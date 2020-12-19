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
2. `vcpkg install gtkmm:x64-windows nlohmann-json:x64-windows ixwebsocket:x64-windows cpr:x64-windows zlib:x64-windows simpleini:x64-windows`
3. `mkdir build && cd build`
4. `cmake -G"Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=c:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVCPKG_TARGET_TRIPLET=x64-windows ..`
5. Build with Visual Studio

#### Mac/Linux:


### Downloads (from CI):
- Windows: [here](https://ci.appveyor.com/project/uowuo/abaddon/build/artifacts)

#### Dependencies:  
* [gtkmm](https://www.gtkmm.org/en/)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [IXWebSocket](https://github.com/machinezone/IXWebSocket)
* [C++ Requests: Curl for People](https://github.com/whoshuu/cpr/)
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
  
.messages - Container of user messages  
.message-container - The container which holds a user's messages  
.message-container-author - The author label for a message container  
.message-container-timestamp - The timestamp label for a message container  
.message-container-extra - Label containing BOT/Webhook  
.message-text - The TextView of a user message  
  
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

