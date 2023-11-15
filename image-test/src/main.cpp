#include <Magick++.h> 
#include <iostream> 

using namespace std; 
using namespace Magick; 

int main(int argc,char **argv) 
{ 
  InitializeMagick(*argv);

  // Construct the image object. Seperating image construction from the 
  // the read operation ensures that a failure to read the image file 
  // doesn't render the image object useless. 
  Image image;
  try { 
    // Read a file into image object 
    image.read("../in/intel_small.png");
    size_t width = image.columns();
    size_t height = image.baseRows();
    int color = 0;
    size_t idx;
    size_t val;

    image.modifyImage();
    image.type(TrueColorType);
    Quantum *pixels = image.getPixels(0, 0, width, height);
    Color green("green");

    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            pixels[width*y+x].red = QuantumRange*green.quantumRed();
            pixels[width*y+x].green = QuantumRange*green.quantumGreen();
            pixels[width*y+x].blue = QuantumRange*green.quantumBlue();
        }
    }

    // Write the image to a file
    image.syncPixels();
    image->write( "logo.png" );
    cout << "Width: " << width << ", Height: " << height << "\n";
    cout << "Done\n";
  } 
  catch( Exception &error_ ) 
    { 
      cout << "Caught exception: " << error_.what() << endl; 
      return 1; 
    } 
  return 0; 
}
