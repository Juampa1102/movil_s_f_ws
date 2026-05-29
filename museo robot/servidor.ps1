# ══════════════════════════════════════════════════════
#  servidor.ps1  —  Servidor HTTP local para Museo Robot
#  No requiere Python, Node.js ni ninguna instalacion
# ══════════════════════════════════════════════════════

$port = 8080
$root = $PSScriptRoot   # misma carpeta que este script

$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add("http://localhost:$port/")

try {
    $listener.Start()
} catch {
    Write-Host ""
    Write-Host " [ERROR] No se pudo iniciar el servidor en el puerto $port." -ForegroundColor Red
    Write-Host " Puede que otro programa este usando ese puerto." -ForegroundColor Yellow
    Write-Host " Cierra esa aplicacion y vuelve a intentarlo." -ForegroundColor Yellow
    Write-Host ""
    Read-Host "Presiona Enter para cerrar"
    exit 1
}

Write-Host ""
Write-Host " ================================================" -ForegroundColor Cyan
Write-Host "   SERVIDOR INICIADO CORRECTAMENTE" -ForegroundColor Green
Write-Host " ================================================" -ForegroundColor Cyan
Write-Host "   URL: http://localhost:$port" -ForegroundColor White
Write-Host "   Presiona Ctrl+C en esta ventana para detener." -ForegroundColor Gray
Write-Host ""

# Abrir navegador automaticamente
Start-Process "http://localhost:$port"

# Tabla de tipos MIME
$mimeTypes = @{
    ".html" = "text/html; charset=utf-8"
    ".css"  = "text/css"
    ".js"   = "application/javascript"
    ".mp4"  = "video/mp4"
    ".webm" = "video/webm"
    ".jpg"  = "image/jpeg"
    ".jpeg" = "image/jpeg"
    ".png"  = "image/png"
    ".webp" = "image/webp"
    ".gif"  = "image/gif"
    ".svg"  = "image/svg+xml"
    ".ico"  = "image/x-icon"
    ".json" = "application/json"
    ".bat"  = "text/plain"
}

Write-Host " Esperando solicitudes..." -ForegroundColor DarkGray
Write-Host ""

while ($listener.IsListening) {
    try {
        $ctx = $listener.GetContext()
        $req = $ctx.Request
        $res = $ctx.Response

        # Decodificar URL
        $urlPath = [System.Uri]::UnescapeDataString($req.Url.LocalPath)
        if ($urlPath -eq "/") { $urlPath = "/index.html" }

        # Construir ruta de archivo
        $filePath = Join-Path $root ($urlPath.TrimStart("/").Replace("/", [System.IO.Path]::DirectorySeparatorChar))

        if (Test-Path $filePath -PathType Leaf) {
            $ext  = [System.IO.Path]::GetExtension($filePath).ToLower()
            $mime = if ($mimeTypes.ContainsKey($ext)) { $mimeTypes[$ext] } else { "application/octet-stream" }

            $res.ContentType = $mime
            $res.StatusCode  = 200

            # Soporte de Range requests para video (streaming)
            $fileInfo  = Get-Item $filePath
            $fileBytes = $null
            $rangeHeader = $req.Headers["Range"]

            if ($rangeHeader -and $mime.StartsWith("video/")) {
                # Parsear rango: bytes=start-end
                $rangeVal = $rangeHeader -replace "bytes=",""
                $parts    = $rangeVal.Split("-")
                $start    = [long]$parts[0]
                $end      = if ($parts[1]) { [long]$parts[1] } else { $fileInfo.Length - 1 }
                $length   = $end - $start + 1

                $res.StatusCode = 206
                $res.AddHeader("Content-Range", "bytes $start-$end/$($fileInfo.Length)")
                $res.AddHeader("Accept-Ranges", "bytes")
                $res.ContentLength64 = $length

                $fs = [System.IO.File]::OpenRead($filePath)
                $fs.Seek($start, [System.IO.SeekOrigin]::Begin) | Out-Null
                $buf = New-Object byte[] $length
                $fs.Read($buf, 0, $length) | Out-Null
                $fs.Close()
                $res.OutputStream.Write($buf, 0, $buf.Length)
            } else {
                $fileBytes = [System.IO.File]::ReadAllBytes($filePath)
                $res.ContentLength64 = $fileBytes.Length
                $res.AddHeader("Accept-Ranges", "bytes")
                $res.OutputStream.Write($fileBytes, 0, $fileBytes.Length)
            }

            Write-Host "  200  $urlPath" -ForegroundColor DarkGreen
        } else {
            $res.StatusCode = 404
            $body = [System.Text.Encoding]::UTF8.GetBytes("404 - Archivo no encontrado: $urlPath")
            $res.ContentLength64 = $body.Length
            $res.OutputStream.Write($body, 0, $body.Length)
            Write-Host "  404  $urlPath" -ForegroundColor DarkYellow
        }

        $res.OutputStream.Close()
    } catch {
        # Ignorar errores de conexion interrumpida (normal en browsers)
    }
}
