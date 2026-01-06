# Setting up GHCR Credentials in Jenkins

## Option 1: Make Image Public (Recommended - Easiest)

1. Go to: https://github.com/PicoPiece?tab=packages
2. Find `ats-node-test` package
3. Click on it → Package settings
4. Scroll to "Danger Zone" → Change visibility → Public

**After making public, no credentials needed!**

---

## Option 2: Use Jenkins Credentials (For Private Images)

### Step 1: Create GitHub Personal Access Token

1. Go to: https://github.com/settings/tokens
2. Click "Generate new token (classic)"
3. Name: `GHCR-Jenkins`
4. Select scopes:
   - ✅ `read:packages` (to pull images)
   - ✅ `write:packages` (if you need to push)
5. Generate token and **copy it** (only shown once)

### Step 2: Add Credentials in Jenkins

1. Go to Jenkins UI: `Manage Jenkins` → `Credentials` → `System` → `Global credentials`
2. Click "Add Credentials"
3. Fill in:
   - **Kind**: `Username with password`
   - **Scope**: `Global`
   - **Username**: Your GitHub username (e.g., `picopiece`)
   - **Password**: Your GitHub Personal Access Token (the token you just created)
   - **ID**: `ghcr-credentials` (or any ID you prefer)
   - **Description**: `GHCR credentials for pulling ats-node-test image`
4. Click "OK"

### Step 3: Use in Pipeline

**Method A: Via Pipeline Parameter**
- When running pipeline, set `GHCR_CREDENTIALS_ID` parameter to your credentials ID (e.g., `ghcr-credentials`)

**Method B: Via Environment Variable**
- Set `GHCR_CREDENTIALS_ID` as environment variable in Jenkins configuration

**Method C: Via JCasC (Configuration as Code)**
- Add to `jenkins.yaml`:
  ```yaml
  credentials:
    system:
      domainCredentials:
        - credentials:
          - usernamePassword:
              id: "ghcr-credentials"
              username: "picopiece"
              password: "${GHCR_TOKEN}"  # Set as environment variable
              description: "GHCR credentials"
  ```

### Step 4: Test

Run pipeline with `GHCR_CREDENTIALS_ID` set to your credentials ID.

---

## Troubleshooting

### "Unauthorized" Error

- **Image is private**: Make it public OR use credentials
- **Wrong credentials ID**: Check credentials ID matches
- **Token expired**: Generate new token
- **Token missing scopes**: Ensure `read:packages` scope

### "Credentials not found"

- Check credentials ID is correct
- Verify credentials exist in Jenkins
- Check credentials scope (should be Global)

### "Login failed"

- Verify token is valid: `gh auth status`
- Test manually: `echo $TOKEN | docker login ghcr.io -u USERNAME --password-stdin`

---

## Recommendation

**For CI/CD, make the image public** - it's simpler and doesn't require credential management on every agent.

