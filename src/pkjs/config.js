module.exports = [
  {
    "type": "heading",
    "defaultValue": "Watch Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Display Options"
      },
      {
        "type": "color",
        "messageKey": "BACKGROUND_COLOR",
        "defaultValue": "000000",
        "label": "Background Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "color",
        "messageKey": "FOREGROUND_COLOR",
        "defaultValue": "FFFFFF",
        "label": "Foreground Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "color",
        "messageKey": "SECONDARY_COLOR",
        "defaultValue": "AAAAAA",
        "label": "Secondary Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "toggle",
        "messageKey": "USE_24_HOUR",
        "label": "Use 24 Hour Time",
        "description": "Switch between 12-hour and 24-hour time format.",
        "defaultValue": true
      },
      {
        "type": "input",
        "messageKey": "STEP_GOAL",
        "defaultValue": "8000",
        "label": "Daily Step Goal",
        "attributes": {
          "type": "number",
          "min": 1000,
          "max": 50000
        }
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
