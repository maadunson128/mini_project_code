// Replace with your Firebase Realtime Database URL and API key
const FIREBASE_URL = '';
const API_KEY = '';

function updateGoogleSheetWithNewData() {
  // Fetch the 'lastID' from Firebase
  const lastIdResponse = UrlFetchApp.fetch(`${FIREBASE_URL}lastID.json?auth=${API_KEY}`);
  const lastId = JSON.parse(lastIdResponse.getContentText());

  // Fetch the 'waterData' from Firebase
  const waterDataResponse = UrlFetchApp.fetch(`${FIREBASE_URL}waterData.json?auth=${API_KEY}`);
  const waterData = JSON.parse(waterDataResponse.getContentText());

  // Open the active Google Sheet and specify the sheet name
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('Sheet1'); // Replace 'Sheet1' with your sheet name

  // Get the last unique ID stored in the Google Sheet (assumed in the first column)
  const lastRow = sheet.getLastRow();
  const lastUniqueIdInSheet = lastRow > 1 ? sheet.getRange(lastRow, 1).getValue() : null;

  // Prepare to append only new data based on the last unique ID
  let newDataToAppend = [];

  for (const uniqueId in waterData) {
    if (waterData.hasOwnProperty(uniqueId)) {
      if (!lastUniqueIdInSheet || uniqueId > lastUniqueIdInSheet) {
        const entry = waterData[uniqueId];
        newDataToAppend.push([
          uniqueId,                // Unique ID
          entry.timestamp,         // Timestamp
          entry.waterLevel,        // Water level in cm
          entry.waterVolume        // Water volume in liters
        ]);
      }
    }
  }

  // Sort the data by unique ID to maintain order
  newDataToAppend.sort((a, b) => a[0].localeCompare(b[0]));

  // Append new data to the sheet
  if (newDataToAppend.length > 0) {
    sheet.getRange(sheet.getLastRow() + 1, 1, newDataToAppend.length, 4).setValues(newDataToAppend);
    Logger.log(`${newDataToAppend.length} new rows added.`);
  } else {
    Logger.log('No new data to append.');
  }
}

