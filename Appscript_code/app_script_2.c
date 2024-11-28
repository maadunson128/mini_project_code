// Replace with your Firebase Realtime Database URL and API key
const FIREBASE_URL = '';
const API_KEY = '';

function updateGoogleSheetWithNewData() {
  // Fetch the 'lastID' from Firebase (most recent entry's ID)
  const lastIdResponse = UrlFetchApp.fetch(`${FIREBASE_URL}lastID.json?auth=${API_KEY}`);
  const lastIdInFirebase = JSON.parse(lastIdResponse.getContentText());

  // Fetch the 'waterData' from Firebase
  const waterDataResponse = UrlFetchApp.fetch(`${FIREBASE_URL}waterData.json?auth=${API_KEY}`);
  const waterData = JSON.parse(waterDataResponse.getContentText());

  // Open the active Google Sheet and specify the sheet names
  const sheet1 = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('Sheet1'); // Data storage
  const sheet2 = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('Sheet2'); // Daily consumption
  const sheet3 = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('Sheet3'); // Monthly consumption

  // Get the last row in Sheet 1
  const lastRow = sheet1.getLastRow();
  const lastUniqueIdInSheet = lastRow > 1 ? sheet1.getRange(lastRow, 1).getValue() : 0; // ID in the last row

  // Check if the last ID in Sheet 1 is the same as the last ID in Firebase
  if (lastUniqueIdInSheet === lastIdInFirebase) {
    Logger.log('No new data to process as the last ID in Sheet 1 matches the last ID in Firebase.');
    return; // No new data, exit the function early
  }

  // Process new data only if there is new data in Firebase
  let newDataToAppend = [];
  let dailyConsumption = 0;
  let previousWaterVolume = null;

  // Process the data from Firebase and update Sheet1
  for (const uniqueId in waterData) {
    if (waterData.hasOwnProperty(uniqueId)) {
      // Only process data that is newer than the last ID in Sheet1
      if (uniqueId > lastUniqueIdInSheet) {
        const entry = waterData[uniqueId];
        const timestamp = entry.timestamp;
        const waterLevel = entry.waterLevel;
        const waterVolume = entry.waterVolume;

        // Add new data to the array for appending to Sheet 1
        newDataToAppend.push([uniqueId, timestamp, waterLevel, waterVolume]);

        // Calculate daily consumption (using negative differences in waterVolume)
        if (previousWaterVolume !== null && waterVolume < previousWaterVolume) {
          dailyConsumption += (previousWaterVolume - waterVolume); // Add positive consumption
        }
        previousWaterVolume = waterVolume;
      }
    }
  }

  // Sort the new data array by unique ID to ensure correct order
  newDataToAppend.sort((a, b) => parseInt(a[0]) - parseInt(b[0]));

  // Append new data to Sheet1 in sorted order (starting from the next row)
  if (newDataToAppend.length > 0) {
    sheet1.getRange(sheet1.getLastRow() + 1, 1, newDataToAppend.length, 4).setValues(newDataToAppend);
    Logger.log(`${newDataToAppend.length} new rows added to Sheet1 in sorted order.`);
  } else {
    Logger.log('No new data to append to Sheet1.');
  }

  // Only proceed with daily and monthly consumption updates if there is consumption data
  if (dailyConsumption > 0) {
    updateDailyAndMonthlyConsumption(sheet2, sheet3, dailyConsumption);
  } else {
    Logger.log('No daily consumption data to update.');
  }
}

function updateDailyAndMonthlyConsumption(sheet2, sheet3, dailyConsumption) {
  const currentDate = Utilities.formatDate(new Date(), Session.getScriptTimeZone(), "yyyy-MM-dd");

  // Update Sheet 2 - Daily Consumption
  const sheet2Data = sheet2.getDataRange().getValues();
  let dateExists = false;

  for (let i = 1; i < sheet2Data.length; i++) { // Start from row 2, as row 1 is headers
    if (sheet2Data[i][0] === currentDate) {
      dateExists = true;
      let existingConsumption = sheet2Data[i][1];
      sheet2.getRange(i + 1, 2).setValue(existingConsumption + dailyConsumption); // Update cumulative consumption
      break;
    }
  }

  // If no entry for today, create a new row for today's consumption
  if (!dateExists) {
    sheet2.appendRow([currentDate, dailyConsumption]);
  }

  // Update Sheet 3 - Monthly Consumption
  const currentMonth = Utilities.formatDate(new Date(), Session.getScriptTimeZone(), "yyyy-MM");
  const sheet3Data = sheet3.getDataRange().getValues();
  let monthExists = false;

  for (let i = 1; i < sheet3Data.length; i++) { // Start from row 2, as row 1 is headers
    if (sheet3Data[i][0] === currentMonth) {
      monthExists = true;
      let existingMonthlyConsumption = sheet3Data[i][1];
      sheet3.getRange(i + 1, 2).setValue(existingMonthlyConsumption + dailyConsumption); // Update cumulative consumption
      break;
    }
  }

  // If no entry for the current month, create a new row for monthly consumption
  if (!monthExists) {
    sheet3.appendRow([currentMonth, dailyConsumption]);
  }
}

