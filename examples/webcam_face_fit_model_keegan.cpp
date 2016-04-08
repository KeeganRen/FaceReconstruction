/*
  
*/

#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>

// using namespace dlib;

#include "eos/core/Landmark.hpp"
#include "eos/core/LandmarkMapper.hpp"
#include "eos/fitting/nonlinear_camera_estimation.hpp"
#include "eos/fitting/linear_shape_fitting.hpp"
#include "eos/render/utils.hpp"
#include "eos/render/texture_extraction.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

#ifdef WIN32
	#define BOOST_ALL_DYN_LINK	// Link against the dynamic boost lib. Seems to be necessary because we use /MD, i.e. link to the dynamic CRT.
	#define BOOST_ALL_NO_LIB	// Don't use the automatic library linking by boost with VS2010 (#pragma ...). Instead, we specify everything in cmake.
#endif
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>


// #include <boost/thread/thread.hpp>
// #include <pcl/common/transforms.h>
// #include <pcl/kdtree/kdtree_flann.h>
// #include <pcl/features/normal_3d.h>
// #include <pcl/visualization/pcl_visualizer.h>
// #include <pcl/surface/texture_mapping.h>
// #include <pcl/io/vtk_lib_io.h>

// using namespace pcl;

using namespace eos;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
using eos::core::Landmark;
using eos::core::LandmarkCollection;
using cv::Mat;
using cv::Vec2f;
using cv::Vec3f;
using cv::Vec4f;
using std::cout;
using std::endl;
using std::vector;
using std::string;

/*
// \brief Display a 3D representation showing the a cloud and a list of camera with their 6DOf poses 
void showCameras (pcl::texture_mapping::CameraVector cams, pcl::PointCloud<pcl::PointXYZ>::Ptr &cloud)
{

  // visualization object
  pcl::visualization::PCLVisualizer visu ("cameras");

  // add a visual for each camera at the correct pose
  for(int i = 0 ; i < cams.size () ; ++i)
  {
    // read current camera
    pcl::TextureMapping<pcl::PointXYZ>::Camera cam = cams[i];
    double focal = cam.focal_length;
    double height = cam.height;
    double width = cam.width;
    
    // create a 5-point visual for each camera
    pcl::PointXYZ p1, p2, p3, p4, p5;
    p1.x=0; p1.y=0; p1.z=0;
    double angleX = RAD2DEG (2.0 * atan (width / (2.0*focal)));
    double angleY = RAD2DEG (2.0 * atan (height / (2.0*focal)));
    double dist = 0.75;
    double minX, minY, maxX, maxY;
    maxX = dist*tan (atan (width / (2.0*focal)));
    minX = -maxX;
    maxY = dist*tan (atan (height / (2.0*focal)));
    minY = -maxY;
    p2.x=minX; p2.y=minY; p2.z=dist;
    p3.x=maxX; p3.y=minY; p3.z=dist;
    p4.x=maxX; p4.y=maxY; p4.z=dist;
    p5.x=minX; p5.y=maxY; p5.z=dist;
    p1=pcl::transformPoint (p1, cam.pose);
    p2=pcl::transformPoint (p2, cam.pose);
    p3=pcl::transformPoint (p3, cam.pose);
    p4=pcl::transformPoint (p4, cam.pose);
    p5=pcl::transformPoint (p5, cam.pose);
    std::stringstream ss;
    ss << "Cam #" << i+1;
    visu.addText3D(ss.str (), p1, 0.1, 1.0, 1.0, 1.0, ss.str ());

    ss.str ("");
    ss << "camera_" << i << "line1";
    visu.addLine (p1, p2,ss.str ());
    ss.str ("");
    ss << "camera_" << i << "line2";
    visu.addLine (p1, p3,ss.str ());
    ss.str ("");
    ss << "camera_" << i << "line3";
    visu.addLine (p1, p4,ss.str ());
    ss.str ("");
    ss << "camera_" << i << "line4";
    visu.addLine (p1, p5,ss.str ());
    ss.str ("");
    ss << "camera_" << i << "line5";
    visu.addLine (p2, p5,ss.str ());
    ss.str ("");
    ss << "camera_" << i << "line6";
    visu.addLine (p5, p4,ss.str ());
    ss.str ("");
    ss << "camera_" << i << "line7";
    visu.addLine (p4, p3,ss.str ());
    ss.str ("");
    ss << "camera_" << i << "line8";
    visu.addLine (p3, p2,ss.str ());
  }
  
  // add a coordinate system
  visu.addCoordinateSystem (1.0, "global");
  
  // add the mesh's cloud (colored on Z axis)
  pcl::visualization::PointCloudColorHandlerGenericField<pcl::PointXYZ> color_handler (cloud, "z");
  visu.addPointCloud (cloud, color_handler, "cloud");
  
  // reset camera
  visu.resetCamera ();
  
  // wait for user input
  visu.spin ();
}

// \brief Helper function that jump to a specific line of a text file 
std::ifstream& GotoLine(std::ifstream& file, unsigned int num)
{
  file.seekg (std::ios::beg);
  for(int i=0; i < num - 1; ++i)
  {
    file.ignore (std::numeric_limits<std::streamsize>::max (),'\n');
  }
  return (file);
}

// \brief Helper function that reads a camera file outputed by Kinfu 
bool readCamPoseFile(std::string filename, pcl::TextureMapping<pcl::PointXYZ>::Camera &cam)
{
  ifstream myReadFile;
  myReadFile.open(filename.c_str (), ios::in);
  if(!myReadFile.is_open ())
  {
    PCL_ERROR ("Error opening file %d\n", filename.c_str ());
    return false;
  }
  myReadFile.seekg(ios::beg);

  char current_line[1024];
  double val;
  
  // go to line 2 to read translations
  GotoLine(myReadFile, 2);
  myReadFile >> val; cam.pose (0,3)=val; //TX
  myReadFile >> val; cam.pose (1,3)=val; //TY
  myReadFile >> val; cam.pose (2,3)=val; //TZ

  // go to line 7 to read rotations
  GotoLine(myReadFile, 7);

  myReadFile >> val; cam.pose (0,0)=val;
  myReadFile >> val; cam.pose (0,1)=val;
  myReadFile >> val; cam.pose (0,2)=val;

  myReadFile >> val; cam.pose (1,0)=val;
  myReadFile >> val; cam.pose (1,1)=val;
  myReadFile >> val; cam.pose (1,2)=val;

  myReadFile >> val; cam.pose (2,0)=val;
  myReadFile >> val; cam.pose (2,1)=val;
  myReadFile >> val; cam.pose (2,2)=val;

  cam.pose (3,0) = 0.0;
  cam.pose (3,1) = 0.0;
  cam.pose (3,2) = 0.0;
  cam.pose (3,3) = 1.0; //Scale
  
  // go to line 12 to read camera focal length and size
  GotoLine (myReadFile, 12);
  myReadFile >> val; cam.focal_length=val; 
  myReadFile >> val; cam.height=val;
  myReadFile >> val; cam.width=val;  
  
  // close file
  myReadFile.close ();

  return true;

}
*/

int main(int argc, char *argv[])
{
    /// read eos file
    fs::path modelfile, isomapfile,/* imagefile, landmarksfile,*/ mappingsfile, outputfilename, outputfilepath;
    try {
	    po::options_description desc("Allowed options");
	    desc.add_options()
		    ("help,h", "display the help message")
		    ("model,m", po::value<fs::path>(&modelfile)->required()->default_value("../share/sfm_shape_3448.bin"), "a Morphable Model stored as cereal BinaryArchive")
// 			("image,i", po::value<fs::path>(&imagefile)->required()->default_value("data/image_0010.png"), "an input image")
// 			("landmarks,l", po::value<fs::path>(&landmarksfile)->required()->default_value("data/image_0010.pts"), "2D landmarks for the image, in ibug .pts format")
		    ("mapping,p", po::value<fs::path>(&mappingsfile)->required()->default_value("../share/ibug2did.txt"), "landmark identifier to model vertex number mapping")
            ("outputfilename,o", po::value<fs::path>(&outputfilename)->required()->default_value("out"), "basename for the output rendering and obj files")
            ("outputfilepath,o", po::value<fs::path>(&outputfilepath)->required()->default_value("output/"), "basename for the output rendering and obj files")
		    ;
	    po::variables_map vm;
	    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
	    if (vm.count("help")) {
		    cout << "Usage: webcam_face_fit_model_keegan [options]" << endl;
		    cout << desc;
		    return EXIT_SUCCESS;
	    }
	    po::notify(vm);
    }
    catch (const po::error& e) {
	    cout << "Error while parsing command-line arguments: " << e.what() << endl;
	    cout << "Use --help to display a list of options." << endl;
	    return EXIT_SUCCESS;
    }
   
    try
    {
	cv::VideoCapture cap(0);
	dlib::image_window win;

	// Load face detection and pose estimation models.
	dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
	dlib::shape_predictor pose_model;
	dlib::deserialize("../share/shape_predictor_68_face_landmarks.dat") >> pose_model;

	// Grab and process frames until the main window is closed by the user.
	int frame_count = 0;
	while(!win.is_closed())
	{
	    // Grab a frame
	    cv::Mat temp;
	    cap >> temp;
	    Mat image;
	    temp.copyTo(image);
	    // Turn OpenCV's Mat into something dlib can deal with.  Note that this just
	    // wraps the Mat object, it doesn't copy anything.  So cimg is only valid as
	    // long as temp is valid.  Also don't do anything to temp that would cause it
	    // to reallocate the memory which stores the image as that will make cimg
	    // contain dangling pointers.  This basically means you shouldn't modify temp
	    // while using cimg.
	    dlib::cv_image<dlib::bgr_pixel> cimg(temp);

	    // Detect faces 
	    std::vector<dlib::rectangle> faces = detector(cimg);
	    /// face rect
// 	    for (size_t i = 0; i < faces.size(); ++i){
// 	      cout << faces[i] << endl;
// 	    }
	    
	    // Find the pose of each face.
	    std::vector<dlib::full_object_detection> shapes;	    
	    for (unsigned long i = 0; i < faces.size(); ++i)
		shapes.push_back(pose_model(cimg, faces[i]));
	    
	    /// face 68 pointers
	    for (size_t i = 0; i < shapes.size(); ++i){
	      ////
	      morphablemodel::MorphableModel morphable_model;
	      try {
		    morphable_model = morphablemodel::load_model(modelfile.string());
	      }
	      catch (const std::runtime_error& e) {
		    cout << "Error loading the Morphable Model: " << e.what() << endl;
		    return EXIT_FAILURE;
	      }
	      core::LandmarkMapper landmark_mapper = mappingsfile.empty() ? core::LandmarkMapper() : core::LandmarkMapper(mappingsfile);
	 
	      /// every face
	      LandmarkCollection<Vec2f> landmarks;
	      landmarks.reserve(68);
	      //cout << "point_num = " << shapes[i].num_parts() << endl;
	      int num_face = shapes[i].num_parts();
	      for (size_t j = 0; j < num_face; ++j){
		dlib::point pt_save = shapes[i].part(j);
		Landmark<Vec2f> landmark;
		/// input
		landmark.name = std::to_string(j+1);
		landmark.coordinates[0] = pt_save.x();
		landmark.coordinates[1] = pt_save.y();
		//cout << shapes[i].part(j) << "\t";
		landmark.coordinates[0] -= 1.0f;
		landmark.coordinates[1] -= 1.0f;
		landmarks.emplace_back(landmark);
	      }
	      //cout << endl;
	      ///////
	      // Draw the loaded landmarks:
	      Mat outimg = image.clone();
// 	      cv::imshow("image", image);
// 	      cv::waitKey(10);
	      int face_point_i = 1;
	      for (auto&& lm : landmarks) {
		      cv::Point numPoint(lm.coordinates[0] - 2.0f, lm.coordinates[1] - 2.0f);
		      cv::rectangle(outimg, cv::Point2f(lm.coordinates[0] - 2.0f, lm.coordinates[1] - 2.0f), cv::Point2f(lm.coordinates[0] + 2.0f, lm.coordinates[1] + 2.0f), { 255, 0, 0 });
		      /// Keegan.Ren
		      /// TODO: plot the face point and point number in the image
		      char str_i[11];
		      sprintf(str_i,"%d",face_point_i);
		      cv::putText(outimg, str_i, numPoint, CV_FONT_HERSHEY_COMPLEX, 0.3, cv::Scalar(0,0,255));
		      ++i;
	      }
	      //cout << "face_point_i = " << face_point_i << endl;
	      cv::imshow("rect_outimg", outimg);
	      cv::waitKey(1);
	      
	      // These will be the final 2D and 3D points used for the fitting:
	      std::vector<Vec4f> model_points; // the points in the 3D shape model
	      std::vector<int> vertex_indices; // their vertex indices
	      std::vector<Vec2f> image_points; // the corresponding 2D landmark points

	      // Sub-select all the landmarks which we have a mapping for (i.e. that are defined in the 3DMM):
	      for (int i = 0; i < landmarks.size(); ++i) {
		      auto converted_name = landmark_mapper.convert(landmarks[i].name);
		      if (!converted_name) { // no mapping defined for the current landmark
			      continue;
		      }
		      int vertex_idx = std::stoi(converted_name.get());
		      Vec4f vertex = morphable_model.get_shape_model().get_mean_at_point(vertex_idx);
		      model_points.emplace_back(vertex);
		      vertex_indices.emplace_back(vertex_idx);
		      image_points.emplace_back(landmarks[i].coordinates);
	      }
	      
	      // Estimate the camera (pose) from the 2D - 3D point correspondences
	      fitting::OrthographicRenderingParameters rendering_params = fitting::estimate_orthographic_camera(image_points, model_points, image.cols, image.rows);
	      Mat affine_from_ortho = get_3x4_affine_camera_matrix(rendering_params, image.cols, image.rows);
	      // Keegan
	      //cout << "affine_from_ortho = " << endl;
	      //cout << affine_from_ortho << endl;
      // 	cv::imshow("affine_from_ortho", affine_from_ortho);
      // 	cv::waitKey();
	      
	      // The 3D head pose can be recovered as follows:
	      float xaw_angle = glm::degrees(rendering_params.r_x);
	      float yaw_angle = glm::degrees(rendering_params.r_y);
	      float zaw_angle = glm::degrees(rendering_params.r_z);
	      //cout << "x_y_z_angle = " << endl;
	      //cout << xaw_angle << "\t" << yaw_angle << "\t" << zaw_angle << endl;
	      // and similarly for pitch (r_x) and roll (r_z).

	      // Estimate the shape coefficients by fitting the shape to the landmarks:
	      std::vector<float> fitted_coeffs = fitting::fit_shape_to_landmarks_linear(morphable_model, affine_from_ortho, image_points, vertex_indices);
// 	      cout << "size = " << fitted_coeffs.size() << endl;
// 	      for (int i = 0; i < fitted_coeffs.size(); ++i)
// 		cout << fitted_coeffs[i] << endl;

	      // Obtain the full mesh with the estimated coefficients:
	      render::Mesh mesh = morphable_model.draw_sample(fitted_coeffs, std::vector<float>());

	      // Extract the texture from the image using given mesh and camera parameters:
	      Mat isomap = render::extract_texture(mesh, affine_from_ortho, image);

	      ///// save obj
	      std::stringstream strOBJ;
          strOBJ << std::setw(10) << std::setfill('0') << frame_count << ".obj";
	      
	      // Save the mesh as textured obj:
          outputfilename = strOBJ.str();
          std::cout << outputfilename << std::endl;
          render::write_textured_obj(mesh, outputfilename.string(), outputfilepath.string());

	      // And save the isomap:
          outputfilename.replace_extension(".isomap.png");
          cv::imwrite(outputfilepath.string() + outputfilename.string(), isomap);

	      cv::imshow("isomap_png", isomap);
	      cv::waitKey(1);
	      
	      //cout << "Finished fitting and wrote result mesh and isomap to files with basename " << outputfile.stem().stem() << "." << endl;
          outputfilename.clear();
	      
	      /*
	      ////////////////////
	        // read mesh from plyfile
	      PCL_INFO ("\nLoading mesh from file %s...\n", argv[1]);
	      pcl::PolygonMesh triangles;
	    //  pcl::io::loadPolygonFilePLY(argv[1], triangles);
	      pcl::io::loadPolygonFileOBJ(strOBJ.str().append((string)".obj"), triangles);

	      pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
	      pcl::fromPCLPointCloud2(triangles.cloud, *cloud);

	      // Create the texturemesh object that will contain our UV-mapped mesh
	      TextureMesh pcl_mesh;
	      pcl_mesh.cloud = triangles.cloud;
	      std::vector< pcl::Vertices> polygon_1;
	      
	      // push faces into the texturemesh object
	      polygon_1.resize (triangles.polygons.size ());
	      for(size_t i =0; i < triangles.polygons.size (); ++i)
	      {
		polygon_1[i] = triangles.polygons[i];
	      }
	      pcl_mesh.tex_polygons.push_back(polygon_1);
	      PCL_INFO ("\tInput mesh contains %d faces and %d vertices\n", pcl_mesh.tex_polygons[0].size (), cloud->points.size ());
	      PCL_INFO ("...Done.\n");
	      
	      // Load textures and cameras poses and intrinsics
	      PCL_INFO ("\nLoading textures and camera poses...\n");
	      pcl::texture_mapping::CameraVector my_cams;
	      
	      const fs::path base_dir (".");
	      std::string extension (".txt");
	      int cpt_cam = 0;
	      for (fs::directory_iterator it (base_dir); it != fs::directory_iterator (); ++it)
	      {
		if(fs::is_regular_file (it->status ()) && fs::extension (it->path ()) == extension)
		{
		  pcl::TextureMapping<pcl::PointXYZ>::Camera cam;
		  readCamPoseFile(it->path ().string (), cam);
		  cam.texture_file = fs::basename (it->path ()) + ".png";
		  my_cams.push_back (cam);
		  cpt_cam++ ;
		}
	      }
	      PCL_INFO ("\tLoaded %d textures.\n", my_cams.size ());
	      PCL_INFO ("...Done.\n");
	      
	      // Display cameras to user
	      PCL_INFO ("\nDisplaying cameras. Press \'q\' to continue texture mapping\n");
	      showCameras(my_cams, cloud);
	      */
	    }
	    frame_count++ ;

	    // Display it all on the screen
	    win.clear_overlay();
	    win.set_image(cimg);
	    win.add_overlay(render_face_detections(shapes));
	}
    }
    catch(dlib::serialization_error& e)
    {
	cout << "You need dlib's default face landmarking model file to run this example." << endl;
	cout << "You can get it from the following URL: " << endl;
	cout << "   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" << endl;
	cout << endl << e.what() << endl;
    }
    catch(std::exception& e)
    {
	cout << e.what() << endl;
    }
    
    return EXIT_SUCCESS;
}

