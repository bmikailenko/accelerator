#ifndef PNG_IMAGE_HPP__
#define PNG_IMAGE_HPP__

#include <vector>
#include <cstdint>
#include <string>
#include <filesystem>
#include <png.h>
#include <assert.h>

#ifdef DEBUG
  #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
#endif

namespace img
{
  enum PNG_BIT_DEPTH : uint8_t {
    ONE     = (1 << 0),
    TWO     = (1 << 1),
    FOUR    = (1 << 2),
    EIGHT   = (1 << 3),
    SIXTEEN = (1 << 4),
  };

  enum PNG_COLOR_TYPE : uint8_t {
    MASK_PALETTE  = 1,
    MASK_COLOR    = 2,
    MASK_ALPHA    = 4,
  };

  enum PNG_COLOR : uint8_t {
    GRAY       = 0,
    PALETTE    = (PNG_COLOR_TYPE::MASK_COLOR | PNG_COLOR_TYPE::MASK_PALETTE),
    RGB        = (PNG_COLOR_TYPE::MASK_COLOR),
    ALPHA      = (PNG_COLOR_TYPE::MASK_COLOR | PNG_COLOR_TYPE::MASK_ALPHA),
    GRAY_ALPHA = (PNG_COLOR_TYPE::MASK_ALPHA),
    UNKNOWN    = 0xFF,
  };

  template<typename T>
  union PNG_PIXEL_RGB {
    T data[3];
    struct {
      T r;
      T g;
      T b;
    } rgb;
  };
  typedef PNG_PIXEL_RGB<uint8_t>           PNG_PIXEL_RGB_8;
  typedef std::vector<PNG_PIXEL_RGB_8>     PNG_PIXEL_RGB_8_ROW;
  typedef std::vector<PNG_PIXEL_RGB_8_ROW> PNG_PIXEL_RGB_8_ROWS;

  typedef PNG_PIXEL_RGB<uint16_t>           PNG_PIXEL_RGB_16;
  typedef std::vector<PNG_PIXEL_RGB_16>     PNG_PIXEL_RGB_16_ROW;
  typedef std::vector<PNG_PIXEL_RGB_16_ROW> PNG_PIXEL_RGB_16_ROWS;

  template<typename T>
  union PNG_PIXEL_RGBA {
    T data[4];
    struct {
      T r;
      T g;
      T b;
      T a;
    } rgba;
  };
  typedef PNG_PIXEL_RGBA<uint8_t>           PNG_PIXEL_RGBA_8;
  typedef std::vector<PNG_PIXEL_RGBA_8>     PNG_PIXEL_RGBA_8_ROW;
  typedef std::vector<PNG_PIXEL_RGBA_8_ROW> PNG_PIXEL_RGBA_8_ROWS;

  typedef PNG_PIXEL_RGBA<uint16_t>           PNG_PIXEL_RGBA_16;
  typedef std::vector<PNG_PIXEL_RGBA_16>     PNG_PIXEL_RGBA_16_ROW;
  typedef std::vector<PNG_PIXEL_RGBA_16_ROW> PNG_PIXEL_RGBA_16_ROWS;

  static inline PNG_PIXEL_RGBA_16 FromRawRGB16ToRGBA16(uint16_t* row, uint32_t column) {
    PNG_PIXEL_RGBA_16 pixel;
    pixel.rgba.r = row[0 + column * 3];
    pixel.rgba.g = row[1 + column * 3];
    pixel.rgba.b = row[2 + column * 3];
    pixel.rgba.a = 0;
    return pixel;
  };

  static inline void FromRGBA16ToRawRGB16(uint16_t* row, uint32_t column, PNG_PIXEL_RGBA_16 pixel) {
    row[0 + column * 3] = pixel.rgba.r;
    row[1 + column * 3] = pixel.rgba.g;
    row[2 + column * 3] = pixel.rgba.b;
  };

  static inline PNG_PIXEL_RGBA_16 FromRawRGBA16ToRGBA16(uint16_t* row, uint32_t column) {
    PNG_PIXEL_RGBA_16 pixel;
    pixel.rgba.r = row[0 + column * 4];
    pixel.rgba.g = row[1 + column * 4];
    pixel.rgba.b = row[2 + column * 4];
    pixel.rgba.a = row[3 + column * 4];
    return pixel;
  };

  static inline void FromRGBA16ToRawRGBA16(uint16_t* row, uint32_t column, PNG_PIXEL_RGBA_16 pixel) {
    row[0 + column * 4] = pixel.rgba.r;
    row[1 + column * 4] = pixel.rgba.g;
    row[2 + column * 4] = pixel.rgba.b;
    row[3 + column * 4] = pixel.rgba.a;
  };

  class PNG {
  public:
    /// @brief Construct PNG structure from file path
    /// @param other
    PNG(std::filesystem::path path) {
      // Check if path is not empty
      if(path.empty() == true)
        throw std::runtime_error("Path cannot be empty");

      // Check that the file exists
      if(std::filesystem::exists(path) == false)
        throw std::runtime_error("File does not exist");

      // Check that the file is a regular file
      if(std::filesystem::is_regular_file(path) == false)
        throw std::runtime_error("Path is not a regular file");

      // Convert relative path to absolute path
      std::filesystem::path absolute_path = std::filesystem::absolute(path);

      // Try and open the file
      std::FILE *fp = std::fopen(absolute_path.c_str(), "rb");
      if(fp == NULL)
        throw std::runtime_error("Could not open file");

      // Load the image from the file
      loadImageFromFile(fp);
      fclose(fp);
    }

    /// @brief Construct PNG structure from memory
    /// @param other
    PNG(std::vector<uint8_t> & data) {
      // Create a file pointer from the data
      FILE * fp = fmemopen((void*)data.data(), data.size(), "rb");
      loadImageFromFile(fp);
      fclose(fp);
    }

    /// @brief Copy Constructor
    /// @param other
    PNG(const PNG& other) {
      m_png        = other.m_png;
      m_info       = other.m_info;
      m_width      = other.m_width;
      m_height     = other.m_height;
      m_channels   = other.m_channels;
      m_color_type = other.m_color_type;
      m_bit_depth  = other.m_bit_depth;
      m_rows       = other.m_rows;
    }

    /// @brief Move Constructor
    /// @param other
    PNG(PNG&& other) {
      m_png        = other.m_png;
      m_info       = other.m_info;
      m_width      = other.m_width;
      m_height     = other.m_height;
      m_channels   = other.m_channels;
      m_color_type = other.m_color_type;
      m_bit_depth  = other.m_bit_depth;
      m_rows       = other.m_rows;

      other.m_png        = nullptr;
      other.m_info       = nullptr;
      other.m_width      = 0;
      other.m_height     = 0;
      other.m_channels   = 0;
      other.m_color_type = static_cast<PNG_COLOR>     (0xFF); // Unknown
      other.m_bit_depth  = static_cast<PNG_BIT_DEPTH> (0xFF); // Unknown
      other.m_rows       = nullptr;
    }

    /// @brief Destructor
    ~PNG(void) {
      png_destroy_read_struct((png_struct**)&m_png, (png_info**)&m_info, NULL);
    }

    uint64_t size(void) const {
      return m_width * m_height * m_channels * (m_bit_depth / 8);
    }

    uint32_t width(void) const {
      return m_width;
    }

    uint32_t height(void) const {
      return m_height;
    }

    uint8_t channels(void) const {
      return m_channels;
    }

    PNG_COLOR colorType(void) const {
      return m_color_type;
    }

    PNG_BIT_DEPTH bitDepth (void) const {
      return m_bit_depth;
    }

    // Converted the image to RGBA16 and return it
    PNG_PIXEL_RGBA_16_ROWS asRGBA16(void) const {
      PNG_PIXEL_RGBA_16_ROWS image_rows = PNG_PIXEL_RGBA_16_ROWS();
      image_rows.reserve(m_height);

      std::function<PNG_PIXEL_RGBA_16(uint16_t*, uint32_t)> pixel_converter;
      switch (m_channels)
      {
      case 3:
        pixel_converter = FromRawRGB16ToRGBA16;
        break;
      case 4:
        pixel_converter = FromRawRGBA16ToRGBA16;
        break;
      default:
        throw std::runtime_error("Unsupported number of channels");
      };

      for(uint32_t row = 0; row < m_height; row++) {
        PNG_PIXEL_RGBA_16_ROW new_row = PNG_PIXEL_RGBA_16_ROW();
        new_row.reserve(m_width);

        for(uint32_t column = 0; column < m_width; column++) {
          new_row.push_back(
            pixel_converter(m_rows[row], column)
          );
        }
        image_rows.push_back(new_row);
      }
      return image_rows;
    }

    // Update the image from RGBA16
    void fromRGBA16(PNG_PIXEL_RGBA_16_ROWS& rows) {
      assert(rows.size()    == m_height && "Image hight doesn't match");
      assert(rows[0].size() == m_width  && "Image width doesn't match");

      std::function<void(uint16_t*, uint32_t, PNG_PIXEL_RGBA_16)> pixel_converter;
      switch (m_channels)
      {
      case 3:
        pixel_converter = FromRGBA16ToRawRGB16;
        break;
      case 4:
        pixel_converter = FromRGBA16ToRawRGBA16;
        break;
      default:
        throw std::runtime_error("Unsupported number of channels");
      };

      for(unsigned int row = 0; row < m_height; row++) {
        for(unsigned int column = 0; column < m_width; column++) {
          pixel_converter(m_rows[row], column, rows[row][column]);
        }
      }
    }

    // Save the file
    void saveToFile(std::filesystem::path path) {
      FILE * fp = fopen(path.c_str(), "wb");
      if(fp == NULL) return;

      png_struct * png  = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

      if(setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, (png_info**)&m_info);
        fclose(fp);
        return;
      }

      png_init_io(png, fp);
      png_write_png(png, (png_info*)m_info, PNG_TRANSFORM_IDENTITY, NULL);
      png_destroy_write_struct(&png, NULL);
      fclose(fp);
    }
  protected:
    void loadImageFromFile(FILE* fp) {
      // Create png structs and info struct
      m_png  = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
      m_info = png_create_info_struct((png_struct*)m_png);

      // Error Handling, this code will only be called if an error occurs
      if(setjmp(png_jmpbuf((png_struct*)m_png))) {
        png_destroy_read_struct((png_struct**)&m_png, (png_info**)&m_info, NULL);
        throw std::runtime_error("Could not parse PNG file");
      }
      // Set the file pointer
      png_init_io((png_struct*)m_png, fp);

      // Read image
      png_read_png((png_struct*)m_png, (png_info*)m_info, PNG_TRANSFORM_EXPAND_16, NULL);

      m_width      = png_get_image_width ((png_struct*)m_png, (png_info*)m_info);
      m_height     = png_get_image_height((png_struct*)m_png, (png_info*)m_info);
      DEBUG_PRINT("Width: %u, Height : %u\n", m_width, m_height);

      m_channels   = png_get_channels((png_struct*)m_png, (png_info*)m_info);
      DEBUG_PRINT("Channels %hhu\n", m_channels);

      m_color_type = static_cast<PNG_COLOR>(png_get_color_type((png_struct*)m_png, (png_info*)m_info));
      DEBUG_PRINT("Color Type %hhu\n", m_color_type);

      m_bit_depth  = static_cast<PNG_BIT_DEPTH>(png_get_bit_depth((png_struct*)m_png, (png_info*)m_info));
      DEBUG_PRINT("Bit Depth %hhu\n", m_bit_depth);

      m_rows       = (uint16_t**)png_get_rows((png_struct*)m_png, (png_info*)m_info);
    }
  private:
    void*         m_png;
    void*         m_info;
    uint32_t      m_width;
    uint32_t      m_height;
    uint8_t       m_channels;
    PNG_COLOR     m_color_type;
    PNG_BIT_DEPTH m_bit_depth;
    uint16_t**    m_rows;
  };
} // namespace image
#endif // PNG_IMAGE_HPP__