#!/usr/bin/env python3
"""
Get Google Calendar Refresh Token

This script authorizes with Google Calendar API and obtains a refresh token
for use with the CYD Clock project.
"""

import json
from pathlib import Path

from google_auth_oauthlib.flow import InstalledAppFlow

# Define the scopes required for your application
SCOPES = ["https://www.googleapis.com/auth/calendar.readonly"]


def get_refresh_token():
    """
    Get a refresh token from Google for Calendar API access.
    """
    # Path to credentials file
    credentials_path = Path("credentials.json")
    token_path = Path("token.json")

    # Check if credentials file exists
    if not credentials_path.exists():
        print(f"Error: {credentials_path} not found.")
        print("Please follow these steps:")
        print("1. Create a Google Cloud project")
        print("2. Enable Calendar API")
        print("3. Create OAuth Client ID credentials for a desktop application")
        print(
            "4. Download the credentials as 'credentials.json' and place it in this directory"
        )
        return

    # Create the flow and run the authorization
    flow = InstalledAppFlow.from_client_secrets_file(credentials_path, scopes=SCOPES)

    # Run the OAuth flow
    credentials = flow.run_local_server(port=0)

    # Extract the credentials into a format we can use
    creds_data = {
        "token": credentials.token,
        "refresh_token": credentials.refresh_token,
        "token_uri": credentials.token_uri,
        "client_id": credentials.client_id,
        "client_secret": credentials.client_secret,
        "scopes": credentials.scopes,
    }

    # Save the credentials to a file
    with token_path.open("w") as token_file:
        json.dump(creds_data, token_file, indent=4)

    print(f"Credentials saved to {token_path}")
    print(f"Refresh Token: {credentials.refresh_token}")

    # Also print the format for secrets.h
    print("\nAdd these lines to src/secrets.h:")
    print('#define GOOGLE_CLIENT_ID "' + credentials.client_id + '"')
    print('#define GOOGLE_CLIENT_SECRET "' + credentials.client_secret + '"')
    print('#define GOOGLE_REFRESH_TOKEN "' + credentials.refresh_token + '"')


if __name__ == "__main__":
    get_refresh_token()
