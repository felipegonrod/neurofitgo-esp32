const express = require("express");
const path = require("path");

const app = express();
const PORT = Number(process.env.PORT) || 3000;
const HOST = process.env.HOST || "0.0.0.0";
const publicDir = path.join(__dirname, "public");

app.disable("x-powered-by");

app.get("/health", (_req, res) => {
  res.type("text/plain").send("ok");
});

app.get("/favicon.ico", (_req, res) => {
  res.status(204).end();
});

app.get(["/", "/index.html"], (_req, res) => {
  res.sendFile(path.join(publicDir, "index.html"));
});

app.get(["/support", "/support/"], (_req, res) => {
  res.sendFile(path.join(publicDir, "support.html"));
});

app.get(["/privacy", "/privacy/"], (_req, res) => {
  res.sendFile(path.join(publicDir, "privacy.html"));
});

app.use(express.static(publicDir, { redirect: false }));

app.use((_req, res) => {
  res.status(404).type("html").send(`<!doctype html>
<html lang="es">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Página no encontrada | NeuroFitGo ESP32</title>
    <link rel="stylesheet" href="/styles.css" />
  </head>
  <body>
    <main class="page">
      <section class="card">
        <p class="eyebrow">NeuroFitGo ESP32</p>
        <h1>Página no encontrada</h1>
        <p>La ruta solicitada no existe.</p>
        <div class="actions">
          <a class="button" href="/">Ir al inicio</a>
        </div>
      </section>
    </main>
  </body>
</html>`);
});

app.listen(PORT, HOST, () => {
  console.log(`NeuroFitGo listening on http://${HOST}:${PORT}`);
});
