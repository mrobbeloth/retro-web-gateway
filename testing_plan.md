# Testing Plan — Retro-fying Web Gateway

Run these tests after every build/deploy to verify all features work correctly.

## Endpoint

```
BASE=http://d3mxaw3jf3qs5.cloudfront.net
```

---

## 1. Portal Homepage

**Test:** Load the root URL and verify the portal page renders with search and URL forms.

```bash
curl -s "$BASE/" | grep -q "Search the Web" && echo "PASS" || echo "FAIL"
```

**Expected:** HTML page with search form, URL form, and bookmarks.

---

## 2. URL Navigation (Direct Path)

**Test:** Fetch a site via the `/proxy/` path format.

```bash
curl -s "$BASE/proxy/http://example.com" | grep -q "Example Domain" && echo "PASS" || echo "FAIL"
```

**Expected:** HTML 3.2 output with page content, no JS/CSS.

---

## 3. URL Navigation (Form Submission / URL-Encoded)

**Test:** Simulate IE5 form submission with percent-encoded URL.

```bash
curl -s "$BASE/proxy/?url=http%3A%2F%2Fexample.com" | grep -q "Example Domain" && echo "PASS" || echo "FAIL"
```

**Expected:** Same result as direct path — URL decoding works.

---

## 4. Search via DuckDuckGo

**Test:** Submit a search query and verify DuckDuckGo results are returned.

```bash
curl -s "$BASE/proxy/?q=retro+computing" | grep -q "duckduckgo" && echo "PASS" || echo "FAIL"
```

**Expected:** DuckDuckGo HTML results with links rewritten through `/proxy/`.

---

## 5. DuckDuckGo Redirect Link Extraction

**Test:** Click a DDG result link (with `uddg=` param) and verify it goes to the actual destination.

```bash
curl -s "$BASE/proxy/http://duckduckgo.com/l/?uddg=https%3A%2F%2Fen.wikipedia.org%2Fwiki%2FRetrocomputing&rut=abc" | grep -q "Retrocomputing" && echo "PASS" || echo "FAIL"
```

**Expected:** Wikipedia Retrocomputing article content, not a DuckDuckGo redirect page.

---

## 6. HTML Simplification

**Test:** Verify JS/CSS is stripped and only HTML 3.2 tags remain.

```bash
RESULT=$(curl -s "$BASE/proxy/http://www.news.com")
echo "$RESULT" | grep -q "<script" && echo "FAIL: script tags present" || echo "PASS: no scripts"
echo "$RESULT" | grep -q "<!DOCTYPE HTML PUBLIC" && echo "PASS: HTML 3.2 doctype" || echo "FAIL: wrong doctype"
```

**Expected:** No `<script>`, `<style>`, `<svg>`, or `<canvas>` tags. HTML 3.2 doctype present.

---

## 7. Link Rewriting

**Test:** Verify all links on a fetched page are rewritten through the proxy.

```bash
curl -s "$BASE/proxy/http://example.com" | grep -q 'href="/proxy/' && echo "PASS" || echo "FAIL"
```

**Expected:** All `href` and `src` attributes prefixed with `/proxy/`.

---

## 8. Image Conversion (JPEG → GIF)

**Test:** Fetch a JPEG image and verify it's converted to GIF.

```bash
curl -s -o /tmp/test_img.gif "$BASE/proxy/https://i.duckduckgo.com/i/e329e62b4aa0b1de.jpg"
file /tmp/test_img.gif | grep -q "GIF image" && echo "PASS" || echo "FAIL"
```

**Expected:** Output is GIF image data, max 320px wide, 256 colors.

---

## 9. Image Conversion (PNG → GIF)

**Test:** Fetch a PNG image and verify it's converted to GIF.

```bash
curl -s -o /tmp/test_png.gif "$BASE/proxy/https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png"
file /tmp/test_png.gif | grep -q "GIF image" && echo "PASS" || echo "FAIL"
```

**Expected:** Output is GIF image data, resized to max 320px wide.

---

## 10. Response Headers (IE5 Compatibility)

**Test:** Verify Content-Type includes charset and Content-Disposition is inline.

```bash
HEADERS=$(curl -sI "$BASE/proxy/http://example.com")
echo "$HEADERS" | grep -qi "charset=iso-8859-1" && echo "PASS: charset" || echo "FAIL: charset"
echo "$HEADERS" | grep -qi "content-disposition: inline" && echo "PASS: disposition" || echo "FAIL: disposition"
```

**Expected:** `Content-Type: text/html; charset=iso-8859-1` and `Content-Disposition: inline`.

---

## 11. Plain HTTP Access (No TLS Required)

**Test:** Verify the endpoint works over plain HTTP (port 80).

```bash
curl -s "http://d3mxaw3jf3qs5.cloudfront.net/proxy/http://example.com" | grep -q "Example Domain" && echo "PASS" || echo "FAIL"
```

**Expected:** Response received without TLS negotiation.

---

## 12. Query String Passthrough

**Test:** Verify query strings on proxied URLs are preserved.

```bash
curl -s "$BASE/proxy/http://httpbin.org/get?foo=bar" | grep -q "foo" && echo "PASS" || echo "FAIL"
```

**Expected:** Target site receives the original query parameters.

---

## 13. Protocol-Relative URL Resolution

**Test:** Verify `//domain/path` links are resolved to `http://domain/path`.

```bash
curl -s "$BASE/proxy/http://www.wikipedia.org" | grep -q 'href="/proxy/http://en.wikipedia.org/' && echo "PASS" || echo "FAIL"
```

**Expected:** Protocol-relative links converted to absolute HTTP URLs through proxy.

---

## Run All Tests

```bash
#!/bin/bash
BASE=http://d3mxaw3jf3qs5.cloudfront.net
PASS=0; FAIL=0

run() {
    if eval "$1" > /dev/null 2>&1; then ((PASS++)); echo "  PASS: $2"
    else ((FAIL++)); echo "  FAIL: $2"; fi
}

echo "=== Retro Web Gateway Test Suite ==="
run 'curl -s "$BASE/" | grep -q "Search the Web"' "Portal homepage"
run 'curl -s "$BASE/proxy/http://example.com" | grep -q "Example Domain"' "Direct URL proxy"
run 'curl -s "$BASE/proxy/?url=http%3A%2F%2Fexample.com" | grep -q "Example Domain"' "URL-encoded form submission"
run 'curl -s "$BASE/proxy/?q=retro+computing" | grep -q "duckduckgo"' "DuckDuckGo search"
run 'curl -s "$BASE/proxy/http://duckduckgo.com/l/?uddg=https%3A%2F%2Fen.wikipedia.org%2Fwiki%2FRetrocomputing&rut=x" | grep -q "Retrocomputing"' "DDG redirect extraction"
run '! curl -s "$BASE/proxy/http://www.news.com" | grep -q "<script"' "No script tags"
run 'curl -s "$BASE/proxy/http://www.news.com" | grep -q "<!DOCTYPE HTML PUBLIC"' "HTML 3.2 doctype"
run 'curl -s "$BASE/proxy/http://example.com" | grep -q "href=\"/proxy/"' "Link rewriting"
run 'curl -s -o /tmp/t.gif "$BASE/proxy/https://i.duckduckgo.com/i/e329e62b4aa0b1de.jpg" && file /tmp/t.gif | grep -q "GIF image"' "JPEG to GIF"
run 'curl -s -o /tmp/t2.gif "$BASE/proxy/https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png" && file /tmp/t2.gif | grep -q "GIF image"' "PNG to GIF"
run 'curl -sI "$BASE/proxy/http://example.com" | grep -qi "charset=iso-8859-1"' "Charset header"
run 'curl -sI "$BASE/proxy/http://example.com" | grep -qi "content-disposition: inline"' "Content-Disposition header"
run 'curl -s "$BASE/proxy/http://www.wikipedia.org" | grep -q "href=\"/proxy/http://en.wikipedia.org/"' "Protocol-relative URL resolution"

echo ""
echo "Results: $PASS passed, $FAIL failed"
```
