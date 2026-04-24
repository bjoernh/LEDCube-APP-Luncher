import CoreGraphics
import Foundation
let list = CGWindowListCopyWindowInfo([.optionOnScreenOnly], kCGNullWindowID) as! [[String: Any]]
for w in list {
    if let name = w["kCGWindowOwnerName"] as? String, name == "cube_launcher",
       let id = w["kCGWindowNumber"] as? Int { print(id); break }
}
