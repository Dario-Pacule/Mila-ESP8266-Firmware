void serialPrint(const char printTyp, const String prop) {
  if (printTyp == '1') {
    Serial.println();
    Serial.print("Operation ID: ");
    Serial.println(prop);
    Serial.println("Irrigation Started!");

    delay(1000);
  }

  if (printTyp == '2') {
    Serial.println("Irrigation completed!");
    Serial.println("===================================");
    Serial.println();
  }

}
