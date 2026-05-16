// PAC (Proxy Auto-Configuration) file for IE5 and vintage browsers
//
// Setup in IE5:
//   Tools > Internet Options > Connections > LAN Settings
//   Check "Use automatic configuration script"
//   Address: http://d3mxaw3jf3qs5.cloudfront.net/proxy.pac
//
// (Host this file on any HTTP server your vintage machine can reach)

function FindProxyForURL(url, host) {
    // Don't proxy requests already going to our gateway
    if (host == "d3mxaw3jf3qs5.cloudfront.net") {
        return "DIRECT";
    }
    // Route everything else through our gateway
    return "PROXY d3mxaw3jf3qs5.cloudfront.net:80";
}
