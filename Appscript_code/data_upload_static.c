// Replace with your Firebase Realtime Database URL and API key
const FIREBASE_URL = 'https://water-monitoring-d4064-default-rtdb.asia-southeast1.firebasedatabase.app/waterData.json';
const API_KEY = 'AIzaSyDCwzjvqjrB5DIN8NQvSq_bsZM_FLs_U1c';

function loadExistingFirebaseData() {
  // Fetch existing data from Firebase
  const response = UrlFetchApp.fetch(`${FIREBASE_URL}?auth=${API_KEY}`);
  const data = JSON.parse(response.getContentText());

  // Open the active Google Sheet and specify the sheet name
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('Sheet1'); // Replace 'Sheet1' with your sheet name

  // Clear the sheet if needed to avoid duplicates
  sheet.clearContents();
  sheet.appendRow(['Unique ID', 'Timestamp', 'Water Level (cm)', 'Water Volume (liters)']); // Updated header row to include Unique ID

  // Iterate through the data and append it to the sheet
  for (const uniqueId in data) {
    if (data.hasOwnProperty(uniqueId)) {
      const entry = data[uniqueId];
      const timestamp = entry.timestamp; // Timestamp as a string

      // Append row (unique ID, timestamp, water level, water volume)
      sheet.appendRow([
        uniqueId,                 // Add the unique ID
        timestamp,                // Use timestamp as a string directly
        entry.waterLevel,         // Water level in cm
        entry.waterVolume         // Water volume in liters
      ]);
    }
  }

  Logger.log('Data loaded successfully!');
}

