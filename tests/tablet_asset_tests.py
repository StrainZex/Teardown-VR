from pathlib import Path
import struct,xml.etree.ElementTree as ET
root=Path('mods/sledge-vr-accessories')
xml=ET.parse(root/'tablet.xml');shape=xml.find('.//vox');scale=float(shape.attrib['scale'])*.1
data=(root/'tablet.vox').read_bytes()
assert data[:4]==b'VOX ' and data[8:12]==b'MAIN'
at=20;chunks={}
while at<len(data):
 name=data[at:at+4];size,children=struct.unpack_from('<II',data,at+4)
 chunks[name]=data[at+12:at+12+size];at+=12+size+children
x,depth,height=struct.unpack('<III',chunks[b'SIZE'])
assert abs(x*scale-.27)<1e-6 and abs(height*scale-.18)<1e-6 and abs(depth*scale-.0225)<1e-6
print('PASS tablet asset dimensions: 27 x 18 x 2.25 cm at prefab scale')
count=struct.unpack_from('<I',chunks[b'XYZI'])[0];vox=[tuple(chunks[b'XYZI'][i:i+4])for i in range(4,len(chunks[b'XYZI']),4)]
assert len(vox)==count==x*depth*height and len(set((a,b,c)for a,b,c,_ in vox))==count
assert all(0<=a<x and 0<=b<depth and 0<=c<height for a,b,c,_ in vox)
print('PASS valid voxel dimensions, count and unique coordinates')
screen=[(a,b,c)for a,b,c,color in vox if color==2]
assert len(screen)==32*20 and all(b==0 and 2<=a<34 and 2<=c<22 for a,b,c in screen)
print('PASS centered 24 x 15 cm screen on front face matches tablet panel')
assert len(xml.findall('.//vox'))==1 and not xml.findall('.//voxbox')
print('PASS single compact shape without oversized box colliders')

