# Google Calendar Setup for CYD Clock

This utility helps you obtain the necessary Google Calendar API credentials for the CYD Clock project.

## Prerequisites

- Python 3.8 or higher
- [uv](https://github.com/astral-sh/uv) package manager
- A Google Cloud project with the Calendar API enabled

## Step 1: Create a Google Cloud Project

1. Go to the [Google Cloud Console](https://console.cloud.google.com/).
2. Click on the project dropdown at the top of the page and then click "New Project".
3. Give your project a name (e.g., "CYD Clock") and click "Create".

## Step 2: Enable the Google Calendar API

1. In the Google Cloud Console, navigate to "APIs & Services" > "Library".
2. Search for "Google Calendar API" and select it.
3. Click "Enable" to activate the API for your project.

## Step 3: Configure OAuth Consent Screen

1. In the Google Cloud Console, go to "APIs & Services" > "OAuth consent screen".
2. Select "External" as the user type (unless you're planning to only use this with Google Workspace accounts).
3. Fill in the required information:
   - App name: "CYD Clock"
   - User support email: Your email address
   - Developer contact information: Your email address
4. Click "Save and Continue".

## Step 4: Add Scopes

1. On the "Scopes" page, click "Add or Remove Scopes".
2. Select the `https://www.googleapis.com/auth/calendar.readonly` scope.
3. Click "Save and Continue".
4. Complete the remaining steps and click "Back to Dashboard".

## Step 5: Create OAuth Client ID

1. In the Google Cloud Console, go to "APIs & Services" > "Credentials".
2. Click "Create Credentials" > "OAuth client ID".
3. Select "Desktop app" as the application type.
4. Name your OAuth client (e.g., "CYD Clock Client").
5. Click "Create".
6. You'll receive your **Client ID** and **Client Secret**. Note these down for later use.
7. Download the credentials JSON file and save it as `credentials.json` in this directory.

## Step 6: Get Refresh Token

Now that you have your credentials.json file, you can use the included Python script to get a refresh token:

1. Install the dependencies using uv:

   ```bash
   uv pip install -e .
   ```

2. Run the script to get your refresh token:

   ```bash
   python get_refresh_token.py
   ```

3. The script will:
   - Open a browser window for you to authorize access to your Google Calendar
   - Save the credentials to `token.json`
   - Display the refresh token in the terminal
   - Show the exact format to add to your `src/secrets.h` file

## Step 7: Update Your CYD Clock Project

1. Open `src/secrets.h` in your CYD clock project.
2. Replace the placeholder values with your actual credentials:
   ```cpp
   // Google Calendar API credentials
   #define GOOGLE_CLIENT_ID "your-client-id-here"
   #define GOOGLE_CLIENT_SECRET "your-client-secret-here"
   #define GOOGLE_REFRESH_TOKEN "your-refresh-token-here"
   ```

## Additional Notes

- **Test Environment**: The first time your app uses the credentials, you might see a warning saying "This app isn't verified." Click "Advanced" and then "Go to [Your App Name] (unsafe)" to proceed.

- **OAuth Token Expiration**: Your refresh token remains valid unless you revoke access or change security settings in your Google account. The access token is automatically refreshed by your code when needed.

- **Publishing Status**: While testing, you can leave your app in "Testing" status. If you decide to share your app with others, you would need to complete verification, but for personal use, this isn't necessary.

- **Security**: Keep your credentials confidential. Don't commit `secrets.h` with your actual credentials to public repositories.

## Troubleshooting

- **"This app isn't verified"** message: This is normal for development apps. Click "Advanced" and then "Go to [Your App Name] (unsafe)" to proceed.
- **Authentication errors**: Make sure your OAuth consent screen is properly configured and that you've added the correct scopes.
- **Token expiration**: Refresh tokens typically don't expire unless you revoke access or change security settings in your Google account.
- **API errors**: Check the Serial monitor for error messages from the HTTP requests.
