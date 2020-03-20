import puppeteer from "puppeteer";
import WebpackDevServer from "webpack-dev-server";
import webpack from "webpack";
import { MochaRemoteServer } from "mocha-remote";

import { importRealmApp } from "./import-realm-app";

import WEBPACK_CONFIG = require("../webpack.config");

const devtools = "DEV_TOOLS" in process.env;

async function run() {
    // Prepare
    const { appId, baseUrl } = await importRealmApp();
    // Start up the Webpack Dev Server
    const compiler = webpack({
        ...(WEBPACK_CONFIG as webpack.Configuration),
        mode: "development",
        plugins: [
            ...WEBPACK_CONFIG.plugins,
            new webpack.DefinePlugin({
                APP_ID: JSON.stringify(appId),
                // Uses the webpack dev servers proxy
                BASE_URL: JSON.stringify("http://localhost:8080")
            })
        ]
    });

    // Start the webpack-dev-server
    const devServer = new WebpackDevServer(compiler, {
        proxy: { "/api": baseUrl }
    });

    await new Promise((resolve, reject) => {
        devServer.listen(8080, "localhost", err => {
            if (err) {
                reject(err);
            } else {
                resolve();
            }
        });
    });

    process.once("exit", () => {
        devServer.close();
    });

    // Start the mocha remote server
    const mochaServer = new MochaRemoteServer(undefined, {
        runOnConnection: true,
        // Only stop after completion if we're not showing dev-tools
        stopAfterCompletion: !devtools
    });

    process.once("exit", () => {
        mochaServer.stop();
    });

    await mochaServer.start();

    // Start up the browser, running the tests
    const browser = await puppeteer.launch({ devtools });

    process.once("exit", () => {
        browser.close();
    });

    // Navigate to the pages served by the webpack dev server
    const page = await browser.newPage();
    await page.goto("http://localhost:8080");
    // Wait for the tests to complete
    await mochaServer.stopped;
}

run().then(
    () => {
        if (!devtools) {
            process.exit(0);
        }
    },
    err => {
        // tslint:disable-next-line:no-console
        console.error(err);
        if (!devtools) {
            process.exit(1);
        }
    }
);
