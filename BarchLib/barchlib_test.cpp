#include <catch2/catch_all.hpp>

#include <algorithm> // for std::fill
#include <array>     // for std::array
#include <iomanip>   // for std::setfill, std::setw
#include <limits>    // for std::numeric_limits
#include <sstream>   // for std::stringstream

#include <barchlib.hpp>

SCENARIO("Bitmap cannot be constructed with 0 width", "[Bitmap]") {
  GIVEN("it is being constructed with 0 with") {
    THEN("it throws an InvalidSize exception") {
      auto const mustThrow = [] { [[maybe_unused]] BarchLib::Bitmap _{0, 32}; };
      REQUIRE_THROWS_AS(mustThrow(), BarchLib::InvalidSize);
    }
  }
}

SCENARIO("Bitmap cannot be constructed with 0 height", "[Bitmap]") {
  GIVEN("it is being constructed with 0 height") {
    THEN("it throws an InvalidSize exception") {
      auto const mustThrow = [] { [[maybe_unused]] BarchLib::Bitmap _{32, 0}; };
      REQUIRE_THROWS_AS(mustThrow(), BarchLib::InvalidSize);
    }
  }
}

SCENARIO("Bitmap size must correspond to the address space", "[Bitmap]") {
  GIVEN("it is being constructed with humongous width and humongous height") {
    THEN("it throws an InvalidSize exception") {
      auto const mustThrow =
          [humongous = std::numeric_limits<std::size_t>::max() >> 8] {
            [[maybe_unused]] BarchLib::Bitmap _{humongous, humongous};
          };
      REQUIRE_THROWS_AS(mustThrow(), BarchLib::InvalidSize);
    }
  }
}

SCENARIO("Bitmap gracefully handles out-of-memory conditions", "[Bitmap]") {
  GIVEN("it is so large it cannot be allocated") {
    THEN("it throws an InvalidSize exception") {
      auto const mustThrow = [large =
                                  std::numeric_limits<std::size_t>::max() / 2] {
        [[maybe_unused]] BarchLib::Bitmap _{large, 2};
      };
      REQUIRE_THROWS_AS(mustThrow(), BarchLib::InvalidSize);
    }
  }
}

SCENARIO("Bitmap background color", "[Bitmap]") {
  GIVEN("it is constructed with white background") {
    BarchLib::Bitmap bitmap{1, 1};
    THEN("its pixel at (0, 0) is white") {
      REQUIRE(bitmap.pixelAt(0, 0) == BarchLib::White);
    }
  }
  GIVEN("it is constructed with black background") {
    BarchLib::Bitmap bitmap{1, 1, BarchLib::Black};
    THEN("its pixel at (0, 0) is black") {
      REQUIRE(bitmap.pixelAt(0, 0) == BarchLib::Black);
    }
  }
}

SCENARIO("Bitmap knows its size", "[Bitmap]") {
  GIVEN("its size is 3x5") {
    BarchLib::Bitmap bitmap{3, 5};
    WHEN("it's asked about its width") {
      THEN("the answer is 3") { REQUIRE(bitmap.width() == 3); }
    }
    WHEN("it's asked about its height") {
      THEN("the answer is 5") { REQUIRE(bitmap.height() == 5); }
    }
    WHEN("it's asked about its pixel count") {
      THEN("the anser is 15") { REQUIRE(bitmap.pixelCount() == 15); }
    }
  }
}

SCENARIO("Bitmap data can be accessed directly", "[Bitmap]") {
  GIVEN("a 1x1 Bitmap") {
    BarchLib::Bitmap bitmap{1, 1};
    THEN("its data is not null") { REQUIRE(bitmap.data() != nullptr); }
    THEN("tis data points to a white pixel") {
      REQUIRE(*bitmap.data() == BarchLib::White);
    }
  }
}

SCENARIO("Bitmap consits of rows", "[Bitmap]") {
  GIVEN("a 2x2 bitmap") {
    BarchLib::Bitmap bitmap{2, 2};
    THEN("its row at Y=0 is 2 pixels wide") {
      REQUIRE(bitmap.rowAt(0).size() == 2);
    }
    THEN("its row at Y=1 is 2 pixels wide") {
      REQUIRE(bitmap.rowAt(1).size() == 2);
    }
  }
}

SCENARIO("Bitmap does not allow out of bounds access", "[Bitmap]") {
  GIVEN("a 2x2 bitmap") {
    BarchLib::Bitmap bitmap{2, 2};
    WHEN("its row is being accessed at Y=3") {
      auto const mustThrow = [&bitmap] {
        [[maybe_unused]] auto const _ = bitmap.rowAt(3);
      };
      THEN("it trhows an InvalidY exception") {
        REQUIRE_THROWS_AS(mustThrow(), BarchLib::InvalidCoordinate);
      }
    }
    WHEN("its pixel is being accessed at X=1, Y=3") {
      auto const mustThrow = [&bitmap] {
        [[maybe_unused]] auto const _ = bitmap.pixelAt(1, 3);
      };
      THEN("it trhows an InvalidY exception") {
        REQUIRE_THROWS_AS(mustThrow(), BarchLib::InvalidCoordinate);
      }
    }
    WHEN("its pixel is being accessed at X=3, Y=1") {
      auto const mustThrow = [&bitmap] {
        [[maybe_unused]] auto const _ = bitmap.pixelAt(3, 1);
      };
      THEN("it trhows an InvalidX exception") {
        REQUIRE_THROWS_AS(mustThrow(), BarchLib::InvalidCoordinate);
      }
    }
  }
}

SCENARIO("BitSet represents a set of bits", "[BitSet][Internal]") {
  GIVEN("A set of 4 bits") {
    BarchLib::Internal::BitSet bitSet{4};
    AND_GIVEN("the first bit is set") {
      bitSet.set(1);
      WHEN("the 2nd bit is set") {
        bitSet.set(2);
        THEN("the 2nd bit is on") { REQUIRE(bitSet.test(2)); }
        THEN("the 1st bit is still on") { REQUIRE(bitSet.test(1)); }
        THEN("the 0th bit is still off") { REQUIRE_FALSE(bitSet.test(0)); }
        THEN("the 3d  bit is still off") { REQUIRE_FALSE(bitSet.test(3)); }
      }
    }
  }
}

SCENARIO("detect empty rows in a Bitmap", "[Bitmap][Internal]") {
  GIVEN("pixels: 0xFF") {
    THEN("they represent an empty row") {
      REQUIRE(
          BarchLib::Internal::isEmpty(std::array<BarchLib::Pixel, 1>{0xFF}));
    }
  }
  GIVEN("pixels: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF") {
    THEN("they represent an empty row") {
      REQUIRE(BarchLib::Internal::isEmpty(std::array<BarchLib::Pixel, 8>{
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));
    }
  }
  GIVEN("pixels: 0xAA") {
    THEN("they represent a nonempty row") {
      REQUIRE_FALSE(
          BarchLib::Internal::isEmpty(std::array<BarchLib::Pixel, 1>{0xAA}));
    }
  }
  GIVEN("pixels: 0xFF 0xFF 0xFF 0xFF 0xDE 0xAD 0xBE 0xEF") {
    THEN("they represent a nonempty row") {
      REQUIRE_FALSE(BarchLib::Internal::isEmpty(std::array<BarchLib::Pixel, 8>{
          0xFF, 0xFF, 0xFF, 0xFF, 0xDE, 0xAD, 0xBE, 0xEF}));
    }
  }
}

SCENARIO("encoding pixels", "[Encoder][Internal]") {
  GIVEN("pixels: 0xff 0xff 0xff 0xff 0x00 0x00 0x00 0x00 0x01 0x01 0x01 0x01") {
    std::array<BarchLib::Pixel, 12> pixels{0xFFU, 0xFFU, 0xFFU, 0xFFU,
                                           0x00U, 0x00U, 0x00U, 0x00U,
                                           0x01U, 0x01U, 0x01U, 0x01U};
    WHEN("they are encoded") {
      BarchLib::Internal::BitSet encodedPixels;
      BarchLib::Internal::Encoder encoder{encodedPixels};
      encoder.encode(pixels);
      THEN("the result is: "
           "01011 0000 0001 0000 0001 0000 0001 0000 0001") {
        if constexpr (sizeof(std::size_t) == sizeof(std::uint64_t)) {
          REQUIRE(encodedPixels.words().size() == 1U);
          // clang-format off
          REQUIRE(
              encodedPixels.words()[0] ==
              0b01011'0000'0001'0000'0001'0000'0001'0000'0001'000000000000000000000000000);
          //                                      fillers ----^^^^^^^^^^^^^^^^^^^^^^^^^^^^
          // clang-format on
        } else {
          REQUIRE(encodedPixels.words().size() == 2U);
          REQUIRE(encodedPixels.words()[0] ==
                  0b01011'0000'0001'0000'0001'0000'0001'000);
          REQUIRE(encodedPixels.words()[1] ==
                  0b00001'0000'0000'0000'0000'0000'0000'000);
          //              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^-- fillters
        }
      }
    }
  }
}

SCENARIO("decoding pixels", "[Decoder][Internal]") {
  GIVEN("bits: 01011 0000 0001 0000 0001 0000 0001 0000 0001") {
    BarchLib::Internal::BitSet encodedPixels;
    for (std::size_t bitIndex = 0; bitIndex < 37; ++bitIndex) {
      if ("0101100000001000000010000000100000001"[bitIndex] == '1') {
        encodedPixels.set(bitIndex);
      }
    }
    WHEN("they are decoded") {
      BarchLib::Internal::Decoder decoder{encodedPixels};
      std::array<BarchLib::Pixel, 12> pixels;
      decoder.decode(pixels);
      THEN("the resulting pixels are: "
           "0xff 0xff 0xff 0xff 0x00 0x00 0x00 0x00 0x01 0x01 0x01 0x01") {
        REQUIRE(pixels == std::array<BarchLib::Pixel, 12>{
                              0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                              0x01, 0x01, 0x01, 0x01});
      }
    }
  }
}

SCENARIO("once constructed, a CompressedBitmap is empty",
         "[CompressedBitmap]") {
  GIVEN("an empty 2x2 compressed bitmap") {
    BarchLib::CompressedBitmap bitmap{2, 2};
    THEN("the row at Y=0 is empty") { REQUIRE(bitmap.isEmptyRowAt(0)); }
    THEN("the row at Y=1 is empty") { REQUIRE(bitmap.isEmptyRowAt(1)); }
  }
}

static inline void fill(const BarchLib::MutablePixels pixels,
                        const BarchLib::Pixel color) {
  std::fill(pixels.begin(), pixels.end(), color);
};

SCENARIO("a Bitmap can be compressed into a CompressedBitmap",
         "[Bitmap][CompressedBitmap]") {
  GIVEN("an uncompressed 1x1 bitmap") {
    BarchLib::Bitmap bitmap{1, 1};
    WHEN("it is compressed") {
      BarchLib::CompressedBitmap compressedBitmap = compress(bitmap);
      THEN("the result is a 1x1 compressed bitmap") {
        REQUIRE(compressedBitmap.width() == 1);
        REQUIRE(compressedBitmap.height() == 1);
      }
    }
  }
  GIVEN("a 4x3 bitmap:"
        "\n 00 00 00 00"
        "\n FF FF FF FF"
        "\n DE AD BE EF") {
    BarchLib::Bitmap bitmap{4, 3};
    fill(bitmap.rowAt(0), BarchLib::Black);
    fill(bitmap.rowAt(1), BarchLib::White);
    bitmap.pixelAt(0, 2) = 0xDEU;
    bitmap.pixelAt(1, 2) = 0xADU;
    bitmap.pixelAt(2, 2) = 0xBEU;
    bitmap.pixelAt(3, 2) = 0xEFU;
    WHEN("it is comporessed") {
      std::string progressLog{};
      BarchLib::CompressedBitmap compressedBitmap =
          compress(bitmap, [&progressLog](const std::size_t currentStep,
                                          const std::size_t totalSteps) {
            progressLog +=
                std::to_string((std::size_t{100} * currentStep) / totalSteps);
            progressLog += "% ";
          });
      THEN("the progress log contains: 0% 33% 66% 100%") {
        REQUIRE(progressLog == "0% 33% 66% 100% ");
      }
    }
  }

  GIVEN("a 4x3 bitmap:"
        "\n 00 00 00 00"
        "\n FF FF FF FF"
        "\n DE AD BE EF"
        "\n was compressed") {
    BarchLib::Bitmap bitmap{4, 3};
    fill(bitmap.rowAt(0), BarchLib::Black);
    fill(bitmap.rowAt(1), BarchLib::White);
    bitmap.pixelAt(0, 2) = 0xDEU;
    bitmap.pixelAt(1, 2) = 0xADU;
    bitmap.pixelAt(2, 2) = 0xBEU;
    bitmap.pixelAt(3, 2) = 0xEFU;
    BarchLib::CompressedBitmap compressedBitmap = compress(bitmap);
    WHEN("it is uncompressed") {
      std::string progressLog{};
      BarchLib::Bitmap uncompressedBitmap = uncompress(
          compressedBitmap, [&progressLog](const std::size_t currentStep,
                                           const std::size_t totalSteps) {
            progressLog +=
                std::to_string((std::size_t{100} * currentStep) / totalSteps);
            progressLog += "% ";
          });
      THEN("the preogress log contains: 0% 33% 66% 100%") {
        REQUIRE(progressLog == "0% 33% 66% 100% ");
        AND_THEN("the uncompressed image is equal to the original") {
          REQUIRE(uncompressedBitmap == bitmap);
        }
      }
    }
  }
}

namespace {

struct FakeFile {
  std::stringstream out;
};

template <typename T> void write(FakeFile &file, const T &value) {
  file.out << std::hex << std::setw(sizeof(value) * 2) << std::setfill('0')
           << value << '\'';
}

template <typename T>
void write(FakeFile &file, const std::span<T const> values) {
  for (auto const &value : values) {
    using ::write;
    write(file, value);
  }
}

} // namespace

SCENARIO("Saving compressed bitmap", "[CompressedBitmap]") {
  GIVEN("a 4x3 bitmap:"
        "\n 00 00 00 00"
        "\n FF FF FF FF"
        "\n DE AD BE EF"
        "\n was compressed") {
    BarchLib::Bitmap bitmap{4, 3};
    fill(bitmap.rowAt(0), BarchLib::Black);
    fill(bitmap.rowAt(1), BarchLib::White);
    bitmap.pixelAt(0, 2) = 0xDEU;
    bitmap.pixelAt(1, 2) = 0xADU;
    bitmap.pixelAt(2, 2) = 0xBEU;
    bitmap.pixelAt(3, 2) = 0xEFU;
    BarchLib::CompressedBitmap compressedBitmap = compress(bitmap);
    WHEN("it is saved") {
      FakeFile file;
      save(file, compressedBitmap);
      if constexpr (sizeof(std::size_t) == sizeof(std::uint64_t)) {
        // We're running on a 64-bit system.
        THEN("the resulting output is: "
             "0000000000000004'0000000000000003'a000000000000000'"
             "0000000000000001'bdeadbeef0000000'") {
          REQUIRE(file.out.str() ==
                  "0000000000000004'0000000000000003'a000000000000000'"
                  "0000000000000001'bdeadbeef0000000'");
        }
      } else {
        // We're running on a 32-bit system.
        THEN("the resulting output is: "
             "00000004'00000003'a0000000'"
             "00000001'bdeadbee'f0000000'") {
          REQUIRE(file.out.str() == "00000004'00000003'a0000000'"
                                    "00000001'bdeadbee'f0000000'");
        }
      }
    }
  }
}
