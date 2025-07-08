import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import express from "express";
import { createServer as createViteServer } from "vite";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

// Mock data
const mockSettings = {
  ssid: "MyHomeWiFi",
  ap_mode: true,
  elegooip: "192.168.1.100",
  timeout: 2000,
};

const mockSensorStatus = {
  stopped: true,
  filamentRunout: false,
  elegoo: {
    mainboardID: "asdf",
    printStatus: 0,
    isPrinting: false,
    currentLayer: 0,
    totalLayer: 0,
    progress: 0,
    currentTicks: 0,
    totalTicks: 0,
    PrintSpeedPct: 100,
    isWebsocketConnected: true,
  },
};

const mockLogs = {
  logs: [
    {
      uuid: "3a2435f0-2d8a-43d5-9ef3-fbc3e66fc91b",
      timestamp: 1750974868,
      message: "Connected to Carbon Centauri",
    },
    {
      uuid: "ee72981d-f02d-4a33-ba9c-12be1e7e7451",
      timestamp: 1750974872,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "2b334b67-0531-40c5-b37f-6ae0d373b455",
      timestamp: 1750974881,
      message: "Checking WiFi connection",
    },
    {
      uuid: "5f438479-cb99-4dba-b418-c2bb9a319ef9",
      timestamp: 1750974900,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "35e700db-6513-4075-a911-56c4d2771f86",
      timestamp: 1750974911,
      message: "Checking WiFi connection",
    },
    {
      uuid: "3e09c743-ba97-46a0-b3fe-961dad504216",
      timestamp: 1750974928,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "5d1ba69b-b22d-4761-b40a-ad0f9e9b71a5",
      timestamp: 1750974929,
      message: "Disconnected from ElegooCC server",
    },
    {
      uuid: "5daffbf9-f93d-413f-890e-3e73f50e255f",
      timestamp: 1750974934,
      message: "Connected to Carbon Centauri",
    },
    {
      uuid: "e555a0f5-00f9-483d-a77c-6e18b4ba11f2",
      timestamp: 1750974941,
      message: "Checking WiFi connection",
    },
    {
      uuid: "7304a06d-c3a4-4ebf-b6ca-b95d35503737",
      timestamp: 1750974956,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "94b8f1df-64e7-4f49-920c-75b4ef23d803",
      timestamp: 1750974971,
      message: "Checking WiFi connection",
    },
    {
      uuid: "52898f9e-197b-410e-91d2-6c7db3021957",
      timestamp: 1750974984,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "3d04ff96-accf-445f-9ec4-c8cee7d15c00",
      timestamp: 1750974995,
      message: "Disconnected from ElegooCC server",
    },
    {
      uuid: "180c2ac4-9667-4f5e-babe-c7cc531a3fc1",
      timestamp: 1750975000,
      message: "Connected to Carbon Centauri",
    },
    {
      uuid: "b4f10406-7b57-4f5e-8cbd-8f734de7f7ba",
      timestamp: 1750975001,
      message: "Checking WiFi connection",
    },
    {
      uuid: "d96dd15e-5e12-429a-a807-fdb2fedb32a8",
      timestamp: 1750975012,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "693dea39-a075-4b3e-9392-b3108f8ae0ec",
      timestamp: 1750975031,
      message: "Checking WiFi connection",
    },
    {
      uuid: "3d8a9b0c-852a-47f2-ad9f-28f06f5f6081",
      timestamp: 1750975040,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "0856568d-e8bf-44f8-84c8-491671998e66",
      timestamp: 1750975061,
      message: "Checking WiFi connection",
    },
    {
      uuid: "5b749b25-c901-4f63-a7ce-0a38f2410d20",
      timestamp: 1750975061,
      message: "Disconnected from ElegooCC server",
    },
    {
      uuid: "1a96fa14-bdd0-4257-b735-44bdeb987080",
      timestamp: 1750975066,
      message: "Connected to Carbon Centauri",
    },
    {
      uuid: "b461bd34-54f9-49e3-ba9a-f467e6c0fe37",
      timestamp: 1750975068,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "bf42eecd-ad33-468a-84ad-7c0edb4f1df8",
      timestamp: 1750975091,
      message: "Checking WiFi connection",
    },
    {
      uuid: "3056a000-826a-42dd-a38f-e7f7205b9840",
      timestamp: 1750975096,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "ec067357-716c-4963-acb4-e49f84802f47",
      timestamp: 1750975121,
      message: "Checking WiFi connection",
    },
    {
      uuid: "05b592ef-64b6-46f4-8f2c-a64c6b1c54e0",
      timestamp: 1750975124,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "b628ba71-4a83-4e07-a1fa-eda2e21be5f8",
      timestamp: 1750975128,
      message: "Disconnected from ElegooCC server",
    },
    {
      uuid: "57223056-a33f-4b7c-a97e-df8fda5ad946",
      timestamp: 1750975133,
      message: "Connected to Carbon Centauri",
    },
    {
      uuid: "ba35912e-1f41-43eb-b488-791803f12fe3",
      timestamp: 1750975151,
      message: "Checking WiFi connection",
    },
    {
      uuid: "f6408257-145c-4898-b8b6-97d0d5aba987",
      timestamp: 1750975152,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "898c4072-fa27-4dc3-8c7e-723cffd3dc2e",
      timestamp: 1750975180,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "9595c179-7d25-4234-ab5c-7af10ecbb15e",
      timestamp: 1750975181,
      message: "Checking WiFi connection",
    },
    {
      uuid: "e4c7afa3-0406-46e0-826b-db6dfaddb43d",
      timestamp: 1750975194,
      message: "Disconnected from ElegooCC server",
    },
    {
      uuid: "6b2596c9-3c9d-4aff-ba57-8bb6ff9a74a5",
      timestamp: 1750975199,
      message: "Connected to Carbon Centauri",
    },
    {
      uuid: "00af6000-0be4-4a1a-aabf-a6d2cb63f0d1",
      timestamp: 1750975208,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "fd7c5eda-8be7-4592-9602-ffd2e50e3c8b",
      timestamp: 1750975211,
      message: "Checking WiFi connection",
    },
    {
      uuid: "54addf54-f215-4bde-ba47-ff3bc3b1ab2e",
      timestamp: 1750975236,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "9ca090bc-b4c5-4068-a6c5-2a9d93dd0415",
      timestamp: 1750975241,
      message: "Checking WiFi connection",
    },
    {
      uuid: "031da149-2cbb-41ca-812f-b37b1d15ed32",
      timestamp: 1750975260,
      message: "Disconnected from ElegooCC server",
    },
    {
      uuid: "dc9f6816-8843-47c6-81fd-d042d2f66623",
      timestamp: 1750975265,
      message: "Connected to Carbon Centauri",
    },
    {
      uuid: "75942742-9b3d-4fa1-8f3d-babf66e2f9d6",
      timestamp: 1750975271,
      message: "Checking WiFi connection",
    },
    {
      uuid: "71e909f0-eedb-4809-bc99-3b0da95ca77a",
      timestamp: 1750975292,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "d56089c3-651a-4000-b90f-76bfff7de524",
      timestamp: 1750975301,
      message: "Checking WiFi connection",
    },
    {
      uuid: "6cb7ead4-33d0-479d-be33-6df00589ae7d",
      timestamp: 1750975320,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "3478736d-5c0c-41b6-ab04-27662d13bb68",
      timestamp: 1750975326,
      message: "Disconnected from ElegooCC server",
    },
    {
      uuid: "66f0812f-f81c-4450-aea9-c13c94a9dc31",
      timestamp: 1750975331,
      message: "Connected to Carbon Centauri",
    },
    {
      uuid: "c575b16b-02f9-44b1-af19-6a2c41f34333",
      timestamp: 1750975331,
      message: "Checking WiFi connection",
    },
    {
      uuid: "9565c842-3236-4501-ba4a-eb1fa693f95f",
      timestamp: 1750975348,
      message: "Sending ping to ElegooCC",
    },
    {
      uuid: "95a1d9a1-591a-4ea6-8c34-3c0512426075",
      timestamp: 1750975361,
      message: "Checking WiFi connection",
    },
    {
      uuid: "fe261bc1-c9e4-484c-8e35-8c94757d2be6",
      timestamp: 1750975376,
      message: "Sending ping to ElegooCC",
    },
  ],
};

async function createServer() {
  const app = express();
  const vite = await createViteServer({
    server: { middlewareMode: true },
    // don't include Vite's default HTML handling middlewares
    appType: "spa",
  });

  app.use("/get_settings", async (req, res) => {
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify(mockSettings));
    return;
  });

  app.use("/sensor_status", async (req, res) => {
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify(mockSensorStatus));
    return;
  });

  app.use("/logs", async (req, res) => {
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify(mockLogs));
    return;
  });

  app.use(vite.middlewares);
  app.listen(5173);
  console.log("Server is running on http://localhost:5173");
}

createServer();
