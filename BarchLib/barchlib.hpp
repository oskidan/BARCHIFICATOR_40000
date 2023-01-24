#ifndef BARCHLIB_HPP
#define BARCHLIB_HPP

#include <array>      // for std::array
#include <concepts>   // for std::same_as
#include <cstddef>    // for std::size_t
#include <cstdint>    // for std::uint8_t
#include <cstring>    // for std::memcmp
#include <exception>  // for std::exception
#include <functional> // for std::function
#include <memory>     // for std::unique_ptr
#include <new>        // for placement new
#include <span>       // for std::span
#include <utility>    // for std::pair
#include <vector>     // for std::vector

namespace BarchLib::inline v1 {

/// Pixel represents a shade of gray in range [0, 256).
using Pixel = std::uint8_t;

constexpr inline Pixel White = 0xFFU;
constexpr inline Pixel Black = 0x00U;

/// InvalidSize will be thrown during Bitmap and CompressedBitmap construction
/// when the requested size cannot be handled.
struct InvalidSize final : std::exception {

  /// Reason tells us why this exception was thrown.
  enum Reason {
    /// Specifies that the requested image size is too small. For example, one
    /// of the given dimensions is 0.
    TooSmall = 0,
    /// Specifies that the requested image size is too large. It means that
    /// image data cannot possibly fit into memory.
    TooLarge = 1,
  };

  InvalidSize(const std::size_t width, const std::size_t height,
              const Reason reason)
      : m_width{width}, m_height{height}, m_reason{reason} {}

  const char *what() const noexcept override;

  std::size_t width() const noexcept { return m_width; }
  std::size_t height() const noexcept { return m_height; }

  Reason reason() const noexcept { return m_reason; }

private:
  std::size_t m_width;
  std::size_t m_height;

  Reason m_reason;
};

/// InvalidX will be thrown when accessing pixels at X coordinate that is
/// outside of the valid range [0, width()).
struct InvalidCoordinate final : std::exception {

  enum Kind { X, Y };

  InvalidCoordinate(const Kind kind, const std::size_t value)
      : m_kind{kind}, m_value{value} {}

  const char *what() const noexcept override;

  std::size_t value() const noexcept { return m_value; }

  Kind kind() const noexcept { return m_kind; }

private:
  Kind m_kind;

  std::size_t m_value;
};

/// MutablePixels represent a reqnge of pixels. The pixels in the range can be
/// modified.
using MutablePixels = std::span<Pixel>;

/// ImmutablePixels represent a range of pixels. The pixels in the range cannot
/// be modified.
using ImmutablePixels = std::span<Pixel const>;

// clang-format off
template <typename T, typename U>
concept Writer = requires(T& writer, const U& value) {
  { write(writer, value) } -> std::same_as<void>;
};

template <typename T, typename U>
concept Reader = requires(T& reader, U& value) {
  { read(reader, value) } -> std::same_as<void>;
};

template<typename T, typename U>
concept SpanWriter = requires(T& writer, std::span<U const> values) {
  { write(writer, values) } -> std::same_as<void>;
};

template <typename T, typename U>
concept SpanReader = requires(T& reader, std::span<U> values) {
  { read(reader, values) } -> std::same_as<void>;
};
// clang-format on

/// The types and functions in this namespace are internal to BarchLib.
/// Please don't use them or your code may break.
namespace Internal {

template <typename T>
concept BitmapSizeWriter = Writer<T, std::size_t>;

template <typename T>
concept BitmapSizeReader = Reader<T, std::size_t>;

/// BitmapSize represents the size of a bitmap.
struct BitmapSize final {

  /// Preconditions:
  /// 	- width and height are not 0;
  /// 	- width and height represent an image that can be stored in memory.
  BitmapSize(std::size_t width, std::size_t height);

  friend bool operator==(const BitmapSize &lhs,
                         const BitmapSize &rhs) noexcept {
    return lhs.width() == rhs.width() && lhs.height() == rhs.height();
  }

  std::size_t width() const noexcept { return m_width; }
  std::size_t height() const noexcept { return m_height; }

  std::size_t pixelCount() const noexcept { return m_width * m_height; }

  friend void load(BitmapSizeReader auto &reader, BitmapSize &size) {
    std::size_t width = 0;
    std::size_t height = 0;
    read(reader, width);
    read(reader, height);
    size = BitmapSize{width, height};
  }

private:
  std::size_t m_width;
  std::size_t m_height;
};

void load(BitmapSizeReader auto &reader, BitmapSize &size);

void save(BitmapSizeWriter auto &writer, const BitmapSize &size) {
  write(writer, size.width());
  write(writer, size.height());
}

} // namespace Internal

/// Bitmap represents an uncompressed grayscale bitmap.
struct [[nodiscard]] Bitmap final {

  /// Construct an empty bitmap. That is, all the pixels in the image have the
  /// same background color.
  /// Preconditions:
  /// 	- width and height are not 0;
  /// 	- width and height represent an image that can be stored in memory.
  Bitmap(std::size_t width, std::size_t height, Pixel background = White);

  Bitmap(const Bitmap &other);

  Bitmap(Bitmap &&other) noexcept;

  ~Bitmap() noexcept = default;

  Bitmap &operator=(const Bitmap &other) {
    this->~Bitmap();
    return *::new (this) Bitmap{other};
  }

  Bitmap &operator=(Bitmap &&other) noexcept {
    this->~Bitmap();
    return *::new (this) Bitmap{other};
  }

  friend bool operator==(const Bitmap &lhs, const Bitmap &rhs) noexcept {
    if (&lhs == &rhs) { return true; }
    if (lhs.m_size != rhs.m_size) { return false; }
    return 0 == std::memcmp(lhs.data(), rhs.data(), lhs.pixelCount());
  }

  std::size_t width() const noexcept { return m_size.width(); }
  std::size_t height() const noexcept { return m_size.height(); }

  std::size_t pixelCount() const noexcept { return m_size.pixelCount(); }

  Pixel *data() noexcept { return m_data.get(); }
  const Pixel *data() const noexcept { return m_data.get(); }

  /// Precondition: y is in range [0, height()).
  [[nodiscard]] MutablePixels rowAt(std::size_t y);

  /// Precondition: y is in range [0, height()).
  [[nodiscard]] ImmutablePixels rowAt(std::size_t y) const;

  /// Preconditions:
  /// - x is in range [0, width());
  /// - y is in range [0, height()).
  [[nodiscard]] Pixel &pixelAt(std::size_t x, std::size_t y);

  /// Preconditions:
  /// - x is in range [0, width());
  /// - y is in range [0, height()).
  [[nodiscard]] Pixel const &pixelAt(std::size_t x, std::size_t y) const;

private:
  /// Holds the size of this Bitmap in pixels.
  Internal::BitmapSize m_size;

  /// Holds the 2D array of pixels that make up the image. There are `m_width *
  /// m_height` pixels in this array.
  std::unique_ptr<Pixel[]> m_data;
};

namespace Internal {

/// Word is a register size unsigned integer.
using Word = std::size_t;

template <typename T>
constexpr inline std::size_t bitsPer = sizeof(T) * std::size_t{8};

constexpr std::size_t align(const std::size_t value,
                            const std::size_t alignment) noexcept {
  const std::size_t maxPadding = alignment - std::size_t{1};
  return (value + maxPadding) & ~maxPadding;
}

template <typename T>
concept BitSetWriter = SpanWriter<T, Word>;

template <typename T>
concept BitSetReader = SpanReader<T, Word>;

/// BitSet represents a set of bits.
struct [[nodiscard]] BitSet final {
  // If you're wondering why we don't use std::vector<bool> instead, reevaluate
  // your life choices.

  BitSet(std::size_t bitCount = 0);

  [[nodiscard]] bool test(std::size_t bitIndex) const;

  void set(std::size_t bitIndex);

  void clear(std::size_t bitIndex);

  std::span<Word const> words() const noexcept { return m_words; }

  std::size_t wordCount() const noexcept { return m_words.size(); }

  /// unsafeResize resizes the underlying vector of words. This is required for
  /// proper BitSet loading. The use of this method elsewhere is discouraged.
  void unsafeResize(const std::size_t wordCount) { m_words.resize(wordCount); }

  friend void load(BitSetReader auto &reader, BitSet &bitSet) {
    read(reader, bitSet.m_words);
  }

private:
  std::vector<Word> m_words;

  static std::pair<std::size_t /* wordIndex */, Word /* bitMask */>
  toWord(std::size_t bitIndex);
};

void load(BitSetReader auto &reader, BitSet &bitSet);

void save(BitSetWriter auto &writer, const BitSet &bitSet) {
  write(writer, bitSet.words());
}

} // namespace Internal

template <typename T>
concept CompressedBitmapWriter =
    Internal::BitmapSizeWriter<T> && Internal::BitSetWriter<T>;

template <typename T>
concept CompressedBitmapReader =
    Internal::BitmapSizeReader<T> && Internal::BitSetReader<T>;

using ProgressHandler = std::function<void(std::size_t /* currentStep */,
                                           std::size_t /* totalSteps */)>;

/// CompressedBitmap represents a Bitmap that was compressed with a fancy-pants
/// algorithm. Almost the famous Middle Out algorithm by Richard Hendricks.
struct [[nodiscard]] CompressedBitmap final {

  /// Constructs an empty compressed bitmap.
  /// Preconditions:
  /// 	- width and height are not 0;
  /// 	- width and height represent an image that can be stored in memory.
  CompressedBitmap(std::size_t width, std::size_t height);

  std::size_t width() const noexcept { return m_size.width(); }
  std::size_t height() const noexcept { return m_size.height(); }

  bool isEmptyRowAt(std::size_t y) const;

  friend CompressedBitmap compress(const Bitmap &sourceBitmap,
                                   ProgressHandler progress);

  friend Bitmap uncompress(const CompressedBitmap &sourceBitmap,
                           ProgressHandler progress);

  friend CompressedBitmap load(CompressedBitmapReader auto &reader) {
    using Internal::load;
    CompressedBitmap bitmap{1, 1};
    load(reader, bitmap.m_size);
    // Read the row lookup table. It's size is dictated by the image haight.
    const std::size_t bitsPerWord = Internal::bitsPer<Internal::Word>;
    std::size_t bitCount = Internal::align(bitmap.height(), bitsPerWord);
    bitmap.m_rowLookupTable.unsafeResize(bitCount / bitsPerWord);
    load(reader, bitmap.m_rowLookupTable);
    // Read pixel data. It's size is stored explicitly in the image.
    std::size_t numDataWords = 0;
    read(reader, numDataWords);
    bitmap.m_pixelData.unsafeResize(numDataWords);
    load(reader, bitmap.m_pixelData);
    return bitmap;
  }

  friend void save(CompressedBitmapWriter auto &writer,
                   const CompressedBitmap &bitmap) {
    save(writer, bitmap.m_size);
    save(writer, bitmap.m_rowLookupTable);
    // Write how many words are occupied by pixel data.
    write(writer, bitmap.m_pixelData.wordCount());
    save(writer, bitmap.m_pixelData);
  }

private:
  /// Holds the size of this CompressedBitmap in pixels.
  Internal::BitmapSize m_size;

  /// Holds one bit per row. The bit determines whether the row is empty. Bits
  /// that correspond to empty rows are off. Bits that correspond to non-empty
  /// rows are on. A row is empty if all of its pixels are white.
  Internal::BitSet m_rowLookupTable;

  /// Holds the encoded data of non-empty rows. The encoding scheme is this:
  /// - 0 	represents 4 contiguous white pixels;
  /// - 10  represents 4 contiguous black pixels;
  /// - 11  starts a sequence of 4 pixels.
  ///   ^^~~~ These are bits.
  Internal::BitSet m_pixelData;
};

CompressedBitmap load(CompressedBitmapReader auto &reader);

CompressedBitmap compress(
    const Bitmap &sourceBitmap,
    ProgressHandler progress = [](const std::size_t /* currentStep */,
                                  const std::size_t /* totalSteps */) {});

Bitmap uncompress(
    const CompressedBitmap &sourceBitmap,
    ProgressHandler progress = [](const std::size_t /* currentStep */,
                                  const std::size_t /* totalSteps */) {});

namespace Internal {

[[nodiscard]] bool isEmpty(const ImmutablePixels pixels);

// PixelBlock represents a combination of four consecutive pixels.
using PixelBlock = std::uint32_t;

constexpr PixelBlock combineLittleEndian(const Pixel p0, const Pixel p1,
                                         const Pixel p2,
                                         const Pixel p3) noexcept {
  PixelBlock result = p0;
  result = (result << 8) | p1;
  result = (result << 8) | p2;
  result = (result << 8) | p3;
  return result;
}

constexpr std::array<Pixel, 4> splitLittleEndian(const PixelBlock block) {
  std::array<Pixel, 4> result;
  result[3] = static_cast<Pixel>(block);
  result[2] = static_cast<Pixel>(block >> 8);
  result[1] = static_cast<Pixel>(block >> 16);
  result[0] = static_cast<Pixel>(block >> 24);
  return result;
}

constexpr PixelBlock combineBigEndian(const Pixel p0, const Pixel p1,
                                      const Pixel p2, const Pixel p3) noexcept {
  PixelBlock result = p3;
  result = (result << 8) | p2;
  result = (result << 8) | p1;
  result = (result << 8) | p0;
  return result;
}

constexpr std::array<Pixel, 4> splitBigEndian(const PixelBlock block) {
  std::array<Pixel, 4> result;
  result[0] = static_cast<Pixel>(block);
  result[1] = static_cast<Pixel>(block >> 8);
  result[2] = static_cast<Pixel>(block >> 16);
  result[3] = static_cast<Pixel>(block >> 24);
  return result;
}

// TODO(oleksii): Test it on BE hardware.
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
constexpr inline auto combine = combineBigEndian;
constexpr inline auto split = splitBigEndian;
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
constexpr inline auto combine = combineLittleEndian;
constexpr inline auto split = splitLittleEndian;
#else
#error "Cannot detect the endianess of your target platform."
#endif

/// Encoder knows how to encode pixels into a stream of bits.
struct [[nodiscard]] Encoder final {

  Encoder(BitSet &output) noexcept : m_output{&output} {}

  void encode(ImmutablePixels pixels);

private:
  BitSet *m_output;

  /// Specifies the position in the stream of bits.
  std::size_t m_index{0};

  void write0(); ///< Writes a clear bit.
  void write1(); ///< Writes a set   bit.

  using WriteBit = void (Encoder::*)();

  constexpr static WriteBit Write[2] = {&Encoder::write0, &Encoder::write1};

  void write(const PixelBlock block);
};

/// Decoder knows how to decode pixels from a stream of bits.
struct [[nodiscard]] Decoder final {

  Decoder(const BitSet &input) noexcept : m_input{&input} {}

  void decode(MutablePixels pixels);

private:
  const BitSet *m_input;

  /// Specifies the position in the stream of bits.
  std::size_t m_index{0};

  /// Returns `true` if the bit is set, `false` otherwise.
  bool readBit();

  PixelBlock read();
};

} // namespace Internal
} // namespace BarchLib::inline v1

#endif // BARCHLIB_HPP
