// PAC (Proxy Auto-Configuration) file for vintage browsers
// Configure your vintage browser to use this PAC file URL, or
// manually set the HTTP proxy to your API Gateway endpoint.
//
// Replace YOUR_API_ID and REGION with your deployed values.

function FindProxyForURL(url, host) {
    // Route all HTTP traffic through the retro proxy
    return "PROXY YOUR_API_ID.execute-api.REGION.amazonaws.com:443";
}
