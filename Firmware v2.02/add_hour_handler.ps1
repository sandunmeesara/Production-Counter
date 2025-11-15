$filePath = "e:\Projects & Self Learning\Tharindu Sir's Projects\Production Counter\Firmware v2.02\code_v3.cpp"
$content = Get-Content $filePath -Raw

# Find the line with "Serial.println(second);" and "return true;" in checkAndSetTimeFromSerial
$findText = @"
  Serial.print("✓ RTC time set to: ");
  Serial.print(year); Serial.print("-");
  Serial.print(month); Serial.print("-");
  Serial.print(day); Serial.print(" ");
  Serial.print(hour); Serial.print(":");
  Serial.print(minute); Serial.print(":");
  Serial.println(second);
  
  return true;
"@

$replaceText = @"
  Serial.print("✓ RTC time set to: ");
  Serial.print(year); Serial.print("-");
  Serial.print(month); Serial.print("-");
  Serial.print(day); Serial.print(" ");
  Serial.print(hour); Serial.print(":");
  Serial.print(minute); Serial.print(":");
  Serial.println(second);
  
  // Check if hour changed - if so, handle it immediately
  if (hour != lastHour) {
    Serial.println("Hour changed - creating hourly file...");
    handleHourChange(newTime);
    lastHour = hour;
  }
  
  return true;
"@

$content = $content -replace [regex]::Escape($findText), $replaceText
$content | Set-Content $filePath

Write-Host "✓ Hour change handler added successfully"
