#version 330 core

in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom Input Uniform
uniform vec3 camPos;
uniform vec3 camDir;
uniform vec2 screenCenter;

#define ZERO 0

// https://learnopengl.com/Advanced-OpenGL/Depth-testing
float CalcDepth(in vec3 rd,in float Idist){
    float local_z=dot(normalize(camDir),rd)*Idist;
    return(1./(local_z)-1./.01)/(1./1000.-1./.01);//计算NDC空间的深度
}

// https://iquilezles.org/articles/distfunctions/
float sdHorseshoe(in vec3 p,in vec2 c,in float r,in float le,vec2 w)
{
    p.x=abs(p.x);
    float l=length(p.xy);
    p.xy=mat2(-c.x,c.y,
    c.y,c.x)*p.xy;
    p.xy=vec2((p.y>0.||p.x>0.)?p.x:l*sign(-c.x),
    (p.x>0.)?p.y:l);
    p.xy=vec2(p.x,abs(p.y-r))-vec2(le,0.);
    
    vec2 q=vec2(length(max(p.xy,0.))+min(0.,max(p.x,p.y)),p.z);
    vec2 d=abs(q)-w;
    return min(max(d.x,d.y),0.)+length(max(d,0.));
}

// r = sphere's radius
// h = cutting's plane's position
// t = thickness
float sdSixWayCutHollowSphere(vec3 p,float r,float h,float t)
{
    // Six way symetry Transformation
    vec3 ap=abs(p);
    if(ap.x<max(ap.y,ap.z)){
        if(ap.y<ap.z)ap.xz=ap.zx;
        else ap.xy=ap.yx;
    }
    
    vec2 q=vec2(length(ap.yz),ap.x);
    
    float w=sqrt(r*r-h*h);
    
    return((h*q.x<w*q.y)?length(q-vec2(w,h)):
    abs(length(q)-r))-t;
}

// https://iquilezles.org/articles/boxfunctions
vec2 iBox(in vec3 ro,in vec3 rd,in vec3 rad)
{
    vec3 m=1./rd;
    vec3 n=m*ro;
    vec3 k=abs(m)*rad;
    vec3 t1=-n-k;
    vec3 t2=-n+k;
    return vec2(max(max(t1.x,t1.y),t1.z),
    min(min(t2.x,t2.y),t2.z));
}

vec2 opU(vec2 d1,vec2 d2)
{
    return(d1.x<d2.x)?d1:d2;
}

//求pos到组合场景的SDF距离
vec2 map(in vec3 pos){
    vec2 res=vec2(sdHorseshoe(pos-vec3(-1.,.08,1.),vec2(cos(1.3),sin(1.3)),.2,.3,vec2(.03,.5)),11.5);
    res=opU(res,vec2(sdSixWayCutHollowSphere(pos-vec3(0.,1.,0.),4.,3.5,.5),4.5));
    return res;
}

// https://www.shadertoy.com/view/Xds3zN
vec2 raycast(in vec3 ro,in vec3 rd){
    vec2 res=vec2(-1.,-1.);
    
    float tmin=1.;
    float tmax=20.;
    
    // raytrace floor plane（和地面求交）
    float tp1=(-ro.y)/rd.y;
    if(tp1>0.)
    {
        tmax=min(tmax,tp1);
        res=vec2(tp1,1.);
    }
    
    float t=tmin;
    for(int i=0;i<70;i++)
    {
        if(t>tmax)break;
        vec2 h=map(ro+rd*t);
        if(abs(h.x)<(.0001*t))
        {
            res=vec2(t,h.y);
            break;
        }
        t+=h.x;
    }
    
    return res;
    
    /**NOTE - raycast算法思路
    场景由组合几何体、地面构成
    先求出光线投射的最长距离，是和地面相交的距离
    然后逐步（70步）沿着射线前进，计算sdf距离
    如果距离小于阈值，则表示相交，返回相交的结果
    */
}

// https://iquilezles.org/articles/rmshadows
float calcSoftshadow(in vec3 ro,in vec3 rd,in float mint,in float tmax){
    // bounding volume
    float tp=(.8-ro.y)/rd.y;if(tp>0.)tmax=min(tmax,tp);
    
    float res=1.;
    float t=mint;
    for(int i=ZERO;i<24;i++)
    {
        float h=map(ro+rd*t).x;
        float s=clamp(8.*h/t,0.,1.);
        res=min(res,s);
        t+=clamp(h,.01,.2);
        if(res<.004||t>tmax)break;
    }
    res=clamp(res,0.,1.);
    return res*res*(3.-2.*res);//三次函数平滑
}

// https://iquilezles.org/articles/normalsSDF
vec3 calcNormal(in vec3 pos){
    vec2 e=vec2(1.,-1.)*.5773*.0005;
    return normalize(e.xyy*map(pos+e.xyy).x+
    e.yyx*map(pos+e.yyx).x+
    e.yxy*map(pos+e.yxy).x+
    e.xxx*map(pos+e.xxx).x);
}

// https://iquilezles.org/articles/nvscene2008/rwwtt.pdf
float calcAO(in vec3 pos,in vec3 nor){
    float occ=0.;
    float sca=1.;
    for(int i=ZERO;i<5;i++)
    {
        float h=.01+.12*float(i)/4.;
        float d=map(pos+h*nor).x;
        occ+=(h-d)*sca;
        sca*=.95;
        if(occ>.35)break;
    }
    return clamp(1.-3.*occ,0.,1.)*(.5+.5*nor.y);
}

// https://iquilezles.org/articles/checkerfiltering
float checkersGradBox(in vec2 p)
{
    // filter kernel
    vec2 w=fwidth(p)+.001;
    // analytical integral (box filter)
    vec2 i=2.*(abs(fract((p-.5*w)*.5)-.5)-abs(fract((p+.5*w)*.5)-.5))/w;
    // xor pattern
    return.5-.5*i.x*i.y;
}

// https://www.shadertoy.com/view/tdS3DG
vec4 render(in vec3 ro,in vec3 rd){
    // background
    vec3 col=vec3(.7,.7,.9)-max(rd.y,0.)*.3;
    
    // raycast scene
    vec2 res=raycast(ro,rd);
    float t=res.x;
    float m=res.y;
    //NOTE - 这里的m只是用来区分材质的：
    // m=1 地面，m=4.5 SixWayCutHollowSphere，m=11.5 Horseshoe
    if(m>-.5){
        vec3 pos=ro+t*rd;
        vec3 nor=(m<1.5)?vec3(0.,1.,0.):calcNormal(pos);
        vec3 ref=reflect(rd,nor);
        
        // material
        col=.2+.2*sin(m*2.+vec3(0.,1.,2.));
        float ks=1.;
        
        if(m<1.5){// ground
            float f=checkersGradBox(3.*pos.xz);
            col=.15+f*vec3(.05);
            ks=.4;
        }
        
        // lighting
        float occ=calcAO(pos,nor);
        
        vec3 lin=vec3(0.);
        
        // sun
        {
            vec3 lig=normalize(vec3(-.5,.4,-.6));
            vec3 hal=normalize(lig-rd);
            
            //diffusion
            float dif=clamp(dot(nor,lig),0.,1.);
            dif*=calcSoftshadow(pos,lig,.02,2.5);//NOTE - 从着色点处向光源发射射线
            
            //specular
            float spe=pow(clamp(dot(nor,hal),0.,1.),16.);//phong模型的高光
            spe*=dif;
            spe*=.04+.96*pow(clamp(1.-dot(hal,lig),0.,1.),5.);//菲涅尔项
            
            lin+=col*2.20*dif*vec3(1.30,1.,.70);
            lin+=5.*spe*vec3(1.30,1.,.70)*ks;
        }
        // sky
        {
            float dif=sqrt(clamp(.5+.5*nor.y,0.,1.));
            dif*=occ;
            
            float spe=smoothstep(-.2,.2,ref.y);
            spe*=dif;
            spe*=.04+.96*pow(clamp(1.+dot(nor,rd),0.,1.),5.);
            spe*=calcSoftshadow(pos,ref,.02,2.5);
            
            lin+=col*.60*dif*vec3(.40,.60,1.15);
            lin+=2.*spe*vec3(.40,.60,1.30)*ks;
        }
        // back
        {
            float dif=clamp(dot(nor,normalize(vec3(.5,0.,.6))),0.,1.)*clamp(1.-pos.y,0.,1.);
            dif*=occ;
            lin+=col*.55*dif*vec3(.25,.25,.25);
        }
        // sss
        {
            float dif=pow(clamp(1.+dot(nor,rd),0.,1.),2.);
            dif*=occ;
            lin+=col*.25*dif*vec3(1.,1.,1.);
        }
        
        col=lin;
        col=mix(col,vec3(.7,.7,.9),1.-exp(-.0001*t*t*t));
    }
    
    return vec4(vec3(clamp(col,0.,1.)),t);
}

// vec3 CalcRayDir(vec2 nCoord){
    //     vec3 horizontal=normalize(cross(camDir,vec3(.0,1.,.0)));
    //     vec3 vertical=normalize(cross(horizontal,camDir));
    //     return normalize(camDir+horizontal*nCoord.x+vertical*nCoord.y);
// }

mat3 setCamera()
{
    vec3 cw=normalize(camDir);
    vec3 cp=vec3(0.,1.,0.);
    vec3 cu=normalize(cross(cw,cp));
    vec3 cv=(cross(cu,cw));
    return mat3(cu,cv,cw);
}

void main()
{
    vec2 nCoord=(gl_FragCoord.xy-screenCenter.xy)/screenCenter.y;
    mat3 ca=setCamera();
    
    // focal length
    float fl=length(camDir);
    vec3 rd=ca*normalize(vec3(nCoord,fl));//世界空间的相机射线方向
    vec3 color=vec3(nCoord/2.+.5,0.);
    float depth=gl_FragCoord.z;
    {
        vec4 res=render(camPos-vec3(0.,0.,0.),rd);
        color=res.xyz;
        depth=CalcDepth(rd,res.w);
    }
    gl_FragColor=vec4(color,1.);
    gl_FragDepth=depth;
}
