# Test script to validate CI workflow syntax
Write-Host "Testing CI workflow files..." -ForegroundColor Green

# Test build-qt-beta.yml
try {
    $content = Get-Content ".github/workflows/build-qt-beta.yml" -Raw
    if ($content -match "name:" -and $content -match "on:" -and $content -match "jobs:") {
        Write-Host "✅ build-qt-beta.yml structure is valid" -ForegroundColor Green
    } else {
        Write-Host "❌ build-qt-beta.yml structure is invalid" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ Error reading build-qt-beta.yml: $($_.Exception.Message)" -ForegroundColor Red
}

# Test build-qt-latest-beta.yml
try {
    $content = Get-Content ".github/workflows/build-qt-latest-beta.yml" -Raw
    if ($content -match "name:" -and $content -match "on:" -and $content -match "jobs:") {
        Write-Host "✅ build-qt-latest-beta.yml structure is valid" -ForegroundColor Green
    } else {
        Write-Host "❌ build-qt-latest-beta.yml structure is invalid" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ Error reading build-qt-latest-beta.yml: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "Workflow validation complete!" -ForegroundColor Green
