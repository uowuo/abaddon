### Abaddon
---
Alternative Discord client made in C++ with GTK

<img src="/.readme/s3.png">

<a href="https://discord.gg/wkCU3vuzG5"><img src="https://discord.com/api/guilds/858156817711890443/widget.png?style=shield"></a>

Current features:
* Not Electron
* Handles most types of chat messages including embeds, images, and replies
* Completely styleable/customizable with CSS (if you have a system GTK theme it won't really use it though)
* Identifies to Discord as the web client unlike other clients so less likely to be falsely flagged as spam<sup>1</sup>
* Set status
* Unread and mention indicators
* Start new DMs and group DMs
* View user profiles (notes, mutual servers, mutual friends)
* Kick, ban, and unban members
* Modify roles and modify members' roles
* Manage invites
* Manage emojis
* View audit log
* Emojis<sup>2</sup>
* Thread support<sup>3</sup>
* Animated avatars, server icons, emojis (can be turned off)
  
1 - Abaddon tries its best to make Discord think it's a legitimate web client. Some of the things done to do this include: using a browser user agent, sending the same IDENTIFY message that the official web client does, using API v9 endpoints in all cases, and not using endpoints the web client does not normally use. There are still a few smaller inconsistencies, however. For example the web client sends lots of telemetry via the `/science` endpoint (uBlock origin stops this) as well as in the headers of all requests. **In any case,** you should use an official client for joining servers, sending new DMs, or managing your friends list if you are afraid of being caught in Discord's spam filters (unlikely).

2 - Unicode emojis are substituted manually as opposed to rendered by GTK on non-Windows platforms. This can be changed with the `stock_emojis` setting as shown at the bottom of this README. A CBDT-based font using Twemoji is provided to allow GTK to render emojis natively on Windows.

3 - There are some inconsistencies with thread state that might be encountered in some more uncommon cases, but they are the result of fundamental issues with Discord's thread implementation.
  
### Building manually (recommended if not on Windows):
#### Windows:
1. `git clone https://github.com/uowuo/abaddon && cd abaddon`
2. `vcpkg install gtkmm:x64-windows nlohmann-json:x64-windows ixwebsocket:x64-windows zlib:x64-windows sqlite3:x64-windows openssl:x64-windows curl:x64-windows`
3. `mkdir build && cd build`
4. `cmake -G"Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=c:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVCPKG_TARGET_TRIPLET=x64-windows ..`
5. Build with Visual Studio  
  
Or, do steps 1 and 2, and open CMakeLists.txt in Visual Studio if `vcpkg integrate install` was run

#### Mac:
1. `git clone https://github.com/uowuo/abaddon && cd abaddon`
2. `brew install gtkmm3 nlohmann-json`
3. `git submodule init && git submodule update subprojects`
4. `mkdir build && cd build`
5. `cmake ..`
6. `make`

#### Linux:
1. Install dependencies: `libgtkmm-3.0-dev`, `libcurl4-gnutls-dev`, and [nlohmann-json](https://github.com/nlohmann/json)
2. `git clone https://github.com/uowuo/abaddon && cd abaddon`
3. `mkdir build && cd build`
4. `cmake ..`
5. `make`

### Downloads:

Latest release version: https://github.com/uowuo/abaddon/releases/latest

**CI:**

- Windows: [here](https://nightly.link/uowuo/abaddon/workflows/ci/master/build-windows-RelWithDebInfo.zip)
- MacOS: [here](https://nightly.link/uowuo/abaddon/workflows/ci/master/build-macos-RelWithDebInfo.zip) unsigned, unpackaged, requires gtkmm3 (e.g. from homebrew)
- Linux: [here](https://nightly.link/uowuo/abaddon/workflows/ci/master/build-linux-MinSizeRel.zip) unpackaged (for now), requires gtkmm3. built on Ubuntu 18.04 + gcc9  

⚠️ If you use Windows, make sure to start from the directory containing `css` and `res`

On Linux, `css` and `res` can also be loaded from `~/.local/share/abaddon` or `/usr/share/abaddon`

`abaddon.ini` will also be automatically used if located at `~/.config/abaddon/abaddon.ini` and there is no `abaddon.ini` in the working directory

#### Dependencies:  
* [gtkmm](https://www.gtkmm.org/en/)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [IXWebSocket](https://github.com/machinezone/IXWebSocket)
* [libcurl](https://curl.se/)
* [zlib](https://zlib.net/)
* [SQLite3](https://www.sqlite.org/index.html)

### TODO:
* Voice support
* User activities
* Nicknames
* More server management stuff
* Manage friends
* A bunch of other stuff

### Styling
#### CSS selectors
.app-window - Applied to all windows. This means the main window and all popups  
.app-popup - Additional class for `.app-window`s when the window is not the main window  
  
.channel-list - Container of the channel list  
  
.messages - Container of user messages  
.message-container - The container which holds a user's messages  
.message-container-author - The author label for a message container  
.message-container-timestamp - The timestamp label for a message container  
.message-container-avatar - Avatar for a user in a message  
.message-container-extra - Label containing BOT/Webhook  
.message-text - The text of a user message  
.pending - Extra class of .message-text for messages pending to be sent  
.failed - Extra class of .message-text for messages that failed to be sent   
.message-attachment-box - Contains attachment info  
.message-reply - Container for the replied-to message in a reply (these elements will also have .message-text set)  
.message-input - Applied to the chat input container  
.replying - Extra class for chat input container when a reply is currently being created  
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
  
.typing-indicator - The typing indicator (also used for replies)  
  
Used in reorderable list implementation:  
.drag-icon
.drag-hover-top
.drag-hover-bottom

Used in guild settings popup:  
.guild-settings-window  
.guild-members-pane-list - Container for list of members in the members pane  
.guild-members-pane-info - Container for member info  
.guild-roles-pane-list - Container for list of roles in the roles pane  
  
Used in profile popup:  
.mutual-friend-item - Applied to every item in the mutual friends list  
.mutual-friend-item-name - Name in mutual friend item  
.mutual-friend-item-avatar - Avatar in mutual friend item   
.mutual-guild-item - Applied to every item in the mutual guilds list  
.mutual-guild-item-name - Name in mutual guild item  
.mutual-guild-item-icon - Icon in mutual guild item  
.mutual-guild-item-nick - User nickname in mutual guild item  
.profile-connection - Applied to every item in the user connections list  
.profile-connection-label - Label in profile connection item  
.profile-connection-check - Checkmark in verified profile connection items  
.profile-connections - Container for profile connections  
.profile-notes - Container for notes in profile window  
.profile-notes-label - Label that says "NOTE"  
.profile-notes-text - Actual note text  
.profile-info-pane - Applied to container for info section of profile popup  
.profile-info-created - Label for creation date of profile  
.user-profile-window  
.profile-main-container - Inner container for profile  
.profile-avatar  
.profile-username  
.profile-switcher - Buttons used to switch viewed section of profile  
.profile-stack - Container for profile info that can be switched between  
.profile-badges - Container for badges  
.profile-badge  
  
### Settings
Settings are configured (for now) by editing abaddon.ini  
The format is similar to the standard Windows ini format **except**:  
* `#` is used to begin comments as opposed to `;`
* Section and key names are case-sensitive

You should edit these while the client is closed even though there's an option to reload while running  
This listing is organized by section.  
For example, memory_db would be set by adding `memory_db = true` under the line `[discord]`

#### discord
* gateway (string) - override url for Discord gateway. must be json format and use zlib stream compression
* api_base (string) - override base url for Discord API
* memory_db (true or false, default false) - if true, Discord data will be kept in memory as opposed to on disk
* token (string) - Discord token used to login, this can be set from the menu
* prefetch (true or false, default false) - if true, new messages will cause the avatar and image attachments to be automatically downloaded

#### http
* user_agent (string) - sets the user-agent to use in HTTP requests to the Discord API (not including media/images)
* concurrent (int, default 20) - how many images can be concurrently retrieved

#### gui
* member_list_discriminator (true or false, default true) - show user discriminators in the member list
* stock_emojis (true or false, default true) - allow abaddon to substitute unicode emojis with images from emojis.bin, must be false to allow GTK to render emojis itself
* custom_emojis (true or false, default true) - download and use custom Discord emojis
* css (string) - path to the main CSS file
* animations (true or false, default true) - use animated images where available (e.g. server icons, emojis, avatars). false means static images will be used
* animated_guild_hover_only (true or false, default true) - only animate guild icons when the guild is being hovered over
* owner_crown (true or false, default true) - show a crown next to the owner
* unreads (true or false, default true) - show unread indicators and mention badges

#### style
* linkcolor (string) - color to use for links in messages
* expandercolor (string) - color to use for the expander in the channel list
* nsfwchannelcolor (string) - color to use for NSFW channels in the channel list
* channelcolor (string) - color to use for SFW channels in the channel list
* mentionbadgecolor (string) - background color for mention badges
* mentionbadgetextcolor (string) - color to use for number displayed on mention badges
* unreadcolor (string) - color to use for the unread indicator

### Environment variables

* ABADDON_NO_FC (Windows only) - don't use custom font config
* ABADDON_CONFIG - change path of configuration file to use. relative to cwd or can be absolute
