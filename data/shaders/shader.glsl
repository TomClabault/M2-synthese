#version 330
 
#ifdef VERTEX_SHADER
// doit calculer la position d'un sommet dans le repere projectif
// indice du sommet : gl_VertexID
void main( )
{
        ;
}
#endif
 
#ifdef FRAGMENT_SHADER
// doit calculer la couleur du fragment
void main( )
{
        gl_FragColor= vec4(0, 1, 1, 1);       // blanc opaque
}
#endif
