package com.ewolff.microservice.catalog.web;

import com.ewolff.microservice.catalog.MinioService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.core.io.ByteArrayResource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;
import org.springframework.web.servlet.mvc.support.RedirectAttributes;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;

@Controller
@RequestMapping("/items")
public class ImageController {

    @Autowired
    private MinioService minioService;

    @PostMapping("/{itemId}/image")
    public String uploadImage(
            @PathVariable Long itemId,
            @RequestParam("file") MultipartFile file,
            RedirectAttributes redirectAttributes) {
        try {
            if (!file.isEmpty()) {
                // Use consistent naming: item-{id}.jpg
                String extension = getFileExtension(file.getOriginalFilename());
                String objectName = "item-" + itemId + extension;
                minioService.uploadFile(objectName, file);
                redirectAttributes.addFlashAttribute("message", "Image uploaded successfully!");
            }
            return "redirect:/catalog/" + itemId + ".html";
        } catch (Exception e) {
            e.printStackTrace();
            redirectAttributes.addFlashAttribute("error", "Failed to upload image: " + e.getMessage());
            return "redirect:/catalog/" + itemId + ".html";
        }
    }
    
    @GetMapping("/{itemId}/image")
    @ResponseBody
    public ResponseEntity<ByteArrayResource> getImage(@PathVariable Long itemId) {
        try {
            // Try common extensions
            String[] extensions = {".jpg", ".jpeg", ".png", ".gif"};
            
            for (String ext : extensions) {
                String objectName = "item-" + itemId + ext;
                if (minioService.fileExists(objectName)) {
                    InputStream inputStream = minioService.getFile(objectName);
                    
                    // Read the entire stream into a byte array
                    ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                    byte[] data = new byte[4096];
                    int nRead;
                    while ((nRead = inputStream.read(data, 0, data.length)) != -1) {
                        buffer.write(data, 0, nRead);
                    }
                    buffer.flush();
                    byte[] imageBytes = buffer.toByteArray();
                    inputStream.close();
                    
                    HttpHeaders headers = new HttpHeaders();
                    headers.setContentType(getMediaType(ext));
                    headers.setContentLength(imageBytes.length);
                    headers.setCacheControl("public, max-age=3600, must-revalidate");
                    headers.set("X-Content-Type-Options", "nosniff");
                    
                    return ResponseEntity.ok()
                            .headers(headers)
                            .body(new ByteArrayResource(imageBytes));
                }
            }
            
            return ResponseEntity.notFound().build();
        } catch (Exception e) {
            e.printStackTrace();
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    @DeleteMapping("/{itemId}/image")
    @ResponseBody
    public ResponseEntity<String> deleteImage(@PathVariable Long itemId) {
        try {
            // Try to delete all possible image formats
            String[] extensions = {".jpg", ".jpeg", ".png", ".gif"};
            boolean deleted = false;
            
            for (String ext : extensions) {
                String objectName = "item-" + itemId + ext;
                if (minioService.fileExists(objectName)) {
                    minioService.deleteFile(objectName);
                    deleted = true;
                }
            }
            
            if (deleted) {
                return ResponseEntity.ok("Image deleted successfully");
            } else {
                return ResponseEntity.notFound().build();
            }
        } catch (Exception e) {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR)
                    .body("Error deleting image: " + e.getMessage());
        }
    }
    
    private String getFileExtension(String filename) {
        if (filename == null || filename.lastIndexOf('.') == -1) {
            return ".jpg";
        }
        return filename.substring(filename.lastIndexOf('.')).toLowerCase();
    }
    
    private MediaType getMediaType(String extension) {
        switch (extension.toLowerCase()) {
            case ".png":
                return MediaType.IMAGE_PNG;
            case ".gif":
                return MediaType.IMAGE_GIF;
            case ".jpeg":
            case ".jpg":
            default:
                return MediaType.IMAGE_JPEG;
        }
    }
}
