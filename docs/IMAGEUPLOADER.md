## Image Uploader
You can drag and drop images to Chatterino or paste them from clipboard to upload them to an external service.

By default, images are uploaded to [i.nuuls.com](https://i.nuuls.com).  
You can change that in `Chatterino Settings -> External Tools -> Image Uploader`.  

Note to advanced users: This module sends multipart-form requests via POST method, so uploading via SFTP/FTP won't work.  
However, popular hosts like [imgur.com](https://imgur.com) are [s-ul.eu](https://s-ul.eu) supported. Scroll down to see example cofiguration.

### General settings explanation:

|Row|Description|
|-|-|
|Request URL|Link to an API endpoint, which is requested by chatterino. Any needed URL parameters should be included here.|
|Form field|Name of a field, which contains image data.|
|Extra headers|Extra headers, that will be included in the request. Header name and value must be separated by colon (`:`). Multiple headers need to be separated with semicolons (`;`).<br>Example: `Authorization: supaKey ; NextHeader: value` .|
|Image link|Schema that tells where is the link in service's response. Leave empty if server's response is just the link itself. Refer to json properties by `{property}`. Supports dot-notation, example: `{property.anotherProperty}` .|
|Deletion link|Same as above.|

<br>

## Examples

### i.nuuls.com

|Row|Description|
|-|-|
|Request URL|`https://i.nuuls.com/upload`|
|Form field|`attachment`|

Other fields empty.

### imgur.com

|Row|Description|
|-|-|
|Request URL|`https://api.imgur.com/3/image`|
|Form field|`image`|
|Extra headers|`Authorization: Client-ID c898c0bb848ca39`|
|Image link|`{data.link}`|
|Deletion link|`https://imgur.com/delete/{data.deletehash}`|

### s-ul.eu

Replace `XXXXXXXXXXXXXXX` with your API key from s-ul.eu. It can be found on [your account's configuration page](https://s-ul.eu/account/configurations).
|Row|Description|
|-|-|
|Request URL|`https://s-ul.eu/api/v1/upload?wizard=true&key=XXXXXXXXXXXXXXX`|
|Form field|`file`|
|Extra headers||
|Image link|`{url}`|
|Deletion link|`https://s-ul.eu/delete.php?file={filename}&key=XXXXXXXXXXXXXXX`|
