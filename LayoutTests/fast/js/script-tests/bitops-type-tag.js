description("Ensure various bit operators correctly tag the final result value");

aDouble = 100000000.5;
shouldBe("aDouble>>27", "0");
shouldBe("aDouble>>27|0", "0");
shouldBe("aDouble>>0", "100000000");
shouldBe("aDouble>>0|0", "100000000");
shouldBe("aDouble|0", "100000000");

var successfullyParsed = true;
