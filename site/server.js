const http = require("http");
const fs = require("fs");
const path = require("path");

const PORT = process.env.PORT || 3000;
const HOST = process.env.HOST || "0.0.0.0";

function layout(title, currentPath, content) {
  const navLink = (href, label) => {
    const active = currentPath === href ? "nav-link active" : "nav-link";
    return `<a href="${href}" class="${active}">${label}</a>`;
  };

  return `<!doctype html>
<html lang="es">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>${title} | NeuroFitGo ESP32</title>
    <meta
      name="description"
      content="Sitio oficial de soporte y privacidad para el proyecto NeuroFitGo ESP32."
    />
    <link rel="stylesheet" href="/styles.css" />
  </head>
  <body>
    <div class="page">
      <header class="hero">
        <p class="eyebrow">NeuroFitGo ESP32</p>
        <h1>${title}</h1>
        <nav class="nav">
          ${navLink("/", "Inicio")}
          ${navLink("/support", "Soporte")}
          ${navLink("/privacy", "Privacidad")}
        </nav>
      </header>
      <main class="card">
        ${content}
      </main>
      <footer class="footer">
        <p>Proyecto NeuroFitGo ESP32</p>
      </footer>
    </div>
  </body>
</html>`;
}

function homePage() {
  return layout(
    "Sitio informativo",
    "/",
    `
      <p>
        Este sitio publica la URL de soporte y la URL de privacidad del proyecto
        <strong>NeuroFitGo ESP32</strong>.
      </p>
      <p>
        Puedes usar estas páginas como referencia pública para distribución,
        documentación o publicación de la app.
      </p>
      <div class="actions">
        <a class="button" href="/support">Ir a soporte</a>
        <a class="button secondary" href="/privacy">Ver privacidad</a>
      </div>
    `
  );
}

function supportPage() {
  return layout(
    "Soporte",
    "/support",
    `
      <p>
        Si necesitas ayuda con el proyecto <strong>NeuroFitGo ESP32</strong>,
        usa este canal de soporte para resolver dudas sobre instalación,
        conexión, uso general o comportamiento del dispositivo.
      </p>
      <h2>Cómo solicitar ayuda</h2>
      <ul>
        <li>Describe el problema con el mayor detalle posible.</li>
        <li>Indica el modelo del dispositivo o entorno que estás usando.</li>
        <li>Explica los pasos para reproducir el problema.</li>
      </ul>
      <h2>Respuesta</h2>
      <p>
        El soporte se ofrece en español y está enfocado en incidencias
        relacionadas con este proyecto. Si esta app fue compartida por un equipo
        o institución, también puedes contactar directamente a ese equipo.
      </p>
    `
  );
}

function privacyPage() {
  return layout(
    "Política de privacidad",
    "/privacy",
    `
      <p>
        En <strong>NeuroFitGo ESP32</strong> valoramos tu privacidad. Esta
        página describe de forma simple cómo se trata la información relacionada
        con el uso del proyecto.
      </p>
      <h2>Información que se puede procesar</h2>
      <ul>
        <li>Datos técnicos necesarios para operar el dispositivo o la app.</li>
        <li>Información básica de diagnóstico cuando se requiere soporte.</li>
      </ul>
      <h2>Uso de la información</h2>
      <ul>
        <li>Mejorar el funcionamiento del proyecto.</li>
        <li>Resolver errores o incidencias técnicas.</li>
        <li>Dar seguimiento a solicitudes de soporte.</li>
      </ul>
      <h2>Compartición de datos</h2>
      <p>
        No compartimos información personal con terceros con fines comerciales.
        Cualquier tratamiento adicional dependerá del entorno donde este
        proyecto sea desplegado.
      </p>
      <h2>Tus derechos</h2>
      <p>
        Puedes solicitar información adicional sobre privacidad a través del
        mismo canal de soporte publicado en este sitio.
      </p>
      <p class="muted">
        Si en el futuro agregas autenticación, analítica o almacenamiento en la
        nube, conviene actualizar esta política para reflejar esos cambios.
      </p>
    `
  );
}

function notFoundPage() {
  return layout(
    "Página no encontrada",
    "",
    `
      <p>La página solicitada no existe.</p>
      <div class="actions">
        <a class="button" href="/">Volver al inicio</a>
      </div>
    `
  );
}

const server = http.createServer((req, res) => {
  const requestPath = req.url || "/";

  if (requestPath === "/favicon.ico") {
    res.writeHead(204);
    res.end();
    return;
  }

  if (requestPath === "/health") {
    res.writeHead(200, { "Content-Type": "text/plain; charset=utf-8" });
    res.end("ok");
    return;
  }

  if (requestPath === "/styles.css") {
    const cssPath = path.join(__dirname, "public", "styles.css");
    fs.readFile(cssPath, "utf8", (error, content) => {
      if (error) {
        res.writeHead(500, { "Content-Type": "text/plain; charset=utf-8" });
        res.end("No se pudo cargar la hoja de estilos.");
        return;
      }

      res.writeHead(200, { "Content-Type": "text/css; charset=utf-8" });
      res.end(content);
    });
    return;
  }

  let html = "";
  let statusCode = 200;

  if (requestPath === "/") {
    html = homePage();
  } else if (requestPath === "/support") {
    html = supportPage();
  } else if (requestPath === "/privacy") {
    html = privacyPage();
  } else {
    statusCode = 404;
    html = notFoundPage();
  }

  res.writeHead(statusCode, { "Content-Type": "text/html; charset=utf-8" });
  res.end(html);
});

server.listen(PORT, HOST, () => {
  console.log(`Sitio disponible en http://${HOST}:${PORT}`);
});
