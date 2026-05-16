# Retro-fying Web Gateway

A C++ AWS Lambda proxy that bridges the modern web with vintage hardware. Configure your old Mac, DOS machine, or retro VM's browser to use this proxy and browse the modern web on period-appropriate hardware.

## How It Works

```
[Vintage Browser] → proxy request → [API Gateway] → [Lambda C++]
                                                         ├─ Fetches target URL (handles modern TLS)
                                                         ├─ Strips JS, CSS, modern HTML5
                                                         ├─ Converts images to 256-color GIF
                                                         └─ Returns HTML 3.2 response
```

The Lambda handles TLS 1.3, HTTP/2, and all the modern protocol complexity so your vintage browser doesn't have to.

## Browser Configuration

### Option A: Manual Proxy (Netscape, IE 3-5, etc.)

Set your HTTP proxy to:
```
Host: YOUR_API_ID.execute-api.REGION.amazonaws.com
Port: 443
```

Then browse to: `http://proxy/http://www.example.com`

### Option B: PAC File

Point your browser's auto-proxy configuration to the `proxy.pac` file (edit it with your deployed endpoint first).

### Option C: Direct URL

Just visit:
```
https://YOUR_API_ID.execute-api.REGION.amazonaws.com/prod/proxy/http://www.example.com
```

## Building

Requires Docker:

```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

## Deploying

```bash
sam deploy --guided
```

## Dependencies

- **libcurl** — Fetches modern web pages with full TLS support
- **Gumbo** — Google's HTML5 parser for robust DOM traversal
- **ImageMagick (Magick++)** — Image resizing and color reduction
- **AWS Lambda C++ Runtime** — Native Lambda execution

## Supported Vintage Targets

- 68k Macs running System 7 with MacWeb or early Netscape
- DOS machines with Arachne or DosLynx
- Windows 3.1/95 with IE 3-5 or Netscape 2-4
- Early Unix workstations with Mosaic or Lynx
- Any system that can speak HTTP/1.0 to a proxy
