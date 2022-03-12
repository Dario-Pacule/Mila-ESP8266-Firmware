int durationCalculator (const RtcDateTime& dt) {
  RtcDateTime now = Rtc.GetDateTime();
  int durationEstimated = 0;

  int hour = now.Hour() - dt.Hour();
  if (hour < 0) {
    hour = (24 - dt.Hour() + now.Hour());

    operationStatus[0] = true;
    operationStatus[1] = true;
    operationStatus[2] = true;
  }

  durationEstimated = 60 * hour;


  int minutes = now.Minute() - dt.Minute();
  if (minutes < 0) {
    minutes = (60 - dt.Minute() + now.Minute());
  }

  durationEstimated += minutes;

  return durationEstimated;
}
